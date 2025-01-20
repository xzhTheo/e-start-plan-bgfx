/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */
#include <vector>
#include <string>
//#include<tchar.h> 

#include <bgfx/bgfx.h>
#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"
#include "bgfx_logo.h"
#include "imgui/imgui.h"
#include <bx/readerwriter.h>
#include <bx/math.h>

#include "entry/entry.h"
#include "entry/cmd.h"
#include "entry/input.h"
#include "camera.h"



namespace
{
	static float s_texelHalf = 0.0f;
	struct Settings
	{
		Settings()
		{
			m_envRotCurr = 0.0f;
			m_envRotDest = 0.0f;
			m_lightCol[0] = 1.0f;
			m_lightCol[1] = 1.0f;
			m_lightCol[2] = 1.0f;
			blinn_phong = true;
			pbr = false;
			m_bgType = 0.0f;
			m_radianceSlider = 2.0f;
			pbr_ibl = false;
			doPrefilterMap = true;
			doBrdfLut = true;
			m_rgbSpec[0] = 1.0f;
			m_rgbSpec[1] = 1.0f;
			m_rgbSpec[2] = 1.0f;
			m_lod = 0.0f;
			m_doDiffuse = false;
			m_doSpecular = false;
			m_doDiffuseIbl = true;
			m_doSpecularIbl = true;
			normalMap = false;
			pcf = false;
			shadowmap = false;
		}

		float m_envRotCurr;
		float m_envRotDest;
		float m_lightCol[3];
		bool blinn_phong;
		bool pbr_ibl;
		float m_radianceSlider;
		float m_bgType;
		bool pbr;
		bool doPrefilterMap;
		bool doBrdfLut;
		float m_rgbSpec[3];
		float m_lod;
		bool  m_doDiffuse;
		bool  m_doSpecular;
		bool  m_doDiffuseIbl;
		bool  m_doSpecularIbl;
		bool  normalMap;
		bool pcf;
		bool shadowmap;
	};

	struct Uniforms
	{
		enum { NumVec4 = 12 };

		void init()
		{
			u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, NumVec4);
		}

		void submit()
		{
			bgfx::setUniform(u_params, m_params, NumVec4);
		}

		void destroy()
		{
			bgfx::destroy(u_params);
		}

		union
		{
			struct
			{
				union
				{
					float m_mtx[16];
					/* 0*/ struct { float m_mtx0[4]; };
					/* 1*/ struct { float m_mtx1[4]; };
					/* 2*/ struct { float m_mtx2[4]; };
					/* 3*/ struct { float m_mtx3[4]; };
				};
				/* 4*/ struct { float blinn_phong, pbr, pbr_ibl, m_bgType; };
				/* 5*/ struct { float normalMap,  shadowmap,pcf,m_unused5[1]; };
				/* 6*/ struct { float m_doDiffuse, m_doSpecular, m_doDiffuseIbl, m_doSpecularIbl; };
				/* 7*/ struct { float m_cameraPos[3], m_unused7[1]; };
				/* 8*/ struct { float doPrefilterMap, doBrdfLut, m_unused8[2];};
				/* 9*/ struct { float m_rgbSpec[4]; };
				/*10*/ struct { float lighingPos[3], m_unused10[1]; };
				/*11*/ struct { float m_lightCol[3], m_unused11[1]; };
			};

			float m_params[NumVec4 * 4];
		};

		bgfx::UniformHandle u_params;
	};
	
	struct PosNormalVertex
	{
		float    m_x;
		float    m_y;
		float    m_z;
		uint32_t m_normal;

		static void init()
		{
			ms_layout
				.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true)
				.end();
		};

		static bgfx::VertexLayout ms_layout;
	};

	bgfx::VertexLayout PosNormalVertex::ms_layout;

	static PosNormalVertex s_hplaneVertices[] =
	{
		{ -1.0f, 0.0f,  1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
		{  1.0f, 0.0f,  1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
		{ -1.0f, 0.0f, -1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
		{  1.0f, 0.0f, -1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
	};
	static const uint16_t s_planeIndices[] =
{
	0, 1, 2,
	1, 3, 2,
};


	/// <summary>
	///  lut   quad
	/// </summary>
	struct PosTexCoord0Vertex
	{
		float m_x;
		float m_y;
		float m_z;

		float m_u;
		float m_v;

		static void init()
		{
			ms_layout
				.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.end();
		}

		static bgfx::VertexLayout ms_layout;
	};
	bgfx::VertexLayout PosTexCoord0Vertex::ms_layout;
	void lutQuad(float _textureWidth, float _textureHeight, bool _originBottomLeft = false, float _width = 1.0f, float _height = 1.0f)
	{
		if (3 == bgfx::getAvailTransientVertexBuffer(3, PosTexCoord0Vertex::ms_layout))
		{
			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 3, PosTexCoord0Vertex::ms_layout);
			PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;

			const float zz = 0.0f;

			const float minx = -_width;
			const float maxx = _width;
			const float miny = 0.0f;
			const float maxy = _height * 2.0f;

			const float texelHalfW = s_texelHalf / _textureWidth;
			const float texelHalfH = s_texelHalf / _textureHeight;
			const float minu = -1.0f + texelHalfW;
			const float maxu = 1.0f + texelHalfW;

			float minv = texelHalfH;
			float maxv = 2.0f + texelHalfH;

			if (_originBottomLeft)
			{
				std::swap(minv, maxv);
				minv -= 1.0f;
				maxv -= 1.0f;
			}

			vertex[0].m_x = minx;
			vertex[0].m_y = miny;
			vertex[0].m_z = zz;
			vertex[0].m_u = minu;
			vertex[0].m_v = minv;

			vertex[1].m_x = maxx;
			vertex[1].m_y = miny;
			vertex[1].m_z = zz;
			vertex[1].m_u = maxu;
			vertex[1].m_v = minv;

			vertex[2].m_x = maxx;
			vertex[2].m_y = maxy;
			vertex[2].m_z = zz;
			vertex[2].m_u = maxu;
			vertex[2].m_v = maxv;

			bgfx::setVertexBuffer(0, &vb);
		}
	}
	struct LightProbe
	{
		enum Enum
		{
			Bolonga,
			Kyoto,

			Count
		};

		void load(const char* _name)
		{
			char filePath[512];

			bx::snprintf(filePath, BX_COUNTOF(filePath), "%s_lod.dds", _name);
			m_tex = loadTexture(filePath, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);

			bx::snprintf(filePath, BX_COUNTOF(filePath), "%s_irr.dds", _name);
			m_texIrr = loadTexture(filePath, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
		}

		void destroy()
		{
			bgfx::destroy(m_tex);
			bgfx::destroy(m_texIrr);
		}

		bgfx::TextureHandle m_tex;
		bgfx::TextureHandle m_texIrr;
	};
	struct Mouse
	{
		Mouse()
			: m_dx(0.0f)
			, m_dy(0.0f)
			, m_prevMx(0.0f)
			, m_prevMy(0.0f)
			, m_scroll(0)
			, m_scrollPrev(0)
		{
		}

		void update(float _mx, float _my, int32_t _mz, uint32_t _width, uint32_t _height)
		{
			const float widthf = float(int32_t(_width));
			const float heightf = float(int32_t(_height));

			// Delta movement.
			m_dx = float(_mx - m_prevMx) / widthf;
			m_dy = float(_my - m_prevMy) / heightf;

			m_prevMx = _mx;
			m_prevMy = _my;

			// Scroll.
			m_scroll = _mz - m_scrollPrev;
			m_scrollPrev = _mz;
		}

		float m_dx; // Screen space.
		float m_dy;
		float m_prevMx;
		float m_prevMy;
		int32_t m_scroll;
		int32_t m_scrollPrev;
	};

	class EStar : public entry::AppI
	{
	public:
		EStar(const char* _name, const char* _description, const char* _url)
			: entry::AppI(_name, _description, _url)

		{
		}

		void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
		{
			Args args(_argc, _argv);

			m_width = _width;
			m_height = _height;
			m_debug = BGFX_DEBUG_NONE;
			m_reset = BGFX_RESET_VSYNC;


			bgfx::Init init;
			init.type = bgfx::RendererType::OpenGL;
			//init.type = bgfx::RendererType::Direct3D11;
			init.vendorId = args.m_pciId;
			init.resolution.width = m_width;
			init.resolution.height = m_height;
			init.resolution.reset = m_reset;
			bgfx::init(init);

			// Enable debug text.
			bgfx::setDebug(m_debug);

			// Set view 0 clear state.
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0 
			);

			// Uniforms.
			m_uniforms.init();

			// Vertex declarations.
			//PosColorTexCoord0Vertex::init();
			PosTexCoord0Vertex::init();
			m_lightProbes[LightProbe::Bolonga].load("..\\resource\\env_maps\\bolonga");
			m_lightProbes[LightProbe::Kyoto].load("..\\resource\\env_maps\\kyoto");
			m_currentLightProbe = LightProbe::Bolonga;
			//blinn phong uniform
	    	s_texturecolor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
			Blinn_Phong = bgfx::createUniform("Blinn_Phong", bgfx::UniformType::Vec4);
			s_ao_rou_meta = bgfx::createUniform("s_ao_rou_meta", bgfx::UniformType::Sampler);
			s_texturenormal = bgfx::createUniform("s_texnormal", bgfx::UniformType::Sampler);
			s_texturecube = bgfx::createUniform("s_texCube", bgfx::UniformType::Sampler);
			s_texturecubeirr = bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler);
			s_textureLUT = bgfx::createUniform("s_texLUT", bgfx::UniformType::Sampler);
			u_depthScaleOffset = bgfx::createUniform("u_depthScaleOffset", bgfx::UniformType::Vec4);
			s_shadowMap = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);
			u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);

			u_mtx = bgfx::createUniform("u_mtx", bgfx::UniformType::Mat4);
			u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
			u_flags = bgfx::createUniform("u_flags", bgfx::UniformType::Vec4);
			u_camPos = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
			uniform_lightPos = bgfx::createUniform("lightPos", bgfx::UniformType::Vec4);
			//u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);
		//  Load diffuse texture.
			m_texturecolor = loadTexture("..\\resource\\pbr_stone\\pbr_stone_base_color.dds");
			m_ao_rou_meta = loadTexture("..\\resource\\pbr_stone\\pbr_stone_aorm.dds");
			m_texturenormal = loadTexture("..\\resource\\pbr_stone\\pbr_stone_normal.dds");
	
			// Get renderer capabilities info.
			const bgfx::Caps* caps = bgfx::getCaps();
			float depthScaleOffset[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
			if (caps->homogeneousDepth)
			{
				depthScaleOffset[0] = 0.5f;
				depthScaleOffset[1] = 0.5f;
			}
			bgfx::setUniform(u_depthScaleOffset, depthScaleOffset);
			bgfx::touch(0);
			cameraCreate();

			// Create vertex stream declaration.
			PosNormalVertex::init();

			// Create program from shaders.
			m_program = loadProgram("vs_pbr_ibl_stone", "fs_pbr_ibl_stone");
			m_programSky = loadProgram("vs_skybox", "fs_skybox");
			shader_plane = loadProgram("vs_plane", "fs_plane");
			shader_shadowmap = loadProgram("vs_shadowmap", "fs_shadowmap");
			shader_LUT = loadProgram("vs_LUT", "fs_LUT");

			m_mesh = meshLoad("..\\resource\\pbr_stone\\pbr_stone_mes.bin");

			m_timeOffset = bx::getHPCounter();

			last_blinn = m_settings.blinn_phong;
			last_pbr = m_settings.pbr;
			last_pbr_ibl = m_settings.pbr_ibl;
			ambient = 0.6f;
			diffuse = 0.18f;
			specular = 0.9f;
			st = 32;

			m_vbh = bgfx::createVertexBuffer(
				bgfx::makeRef(s_hplaneVertices, sizeof(s_hplaneVertices))
				, PosNormalVertex::ms_layout
			);

			m_ibh = bgfx::createIndexBuffer(
				bgfx::makeRef(s_planeIndices, sizeof(s_planeIndices))
			);
			m_shadowMapFB = BGFX_INVALID_HANDLE;
			LUTFBO = BGFX_INVALID_HANDLE;

			m_shadowSamplerSupported = 0 != (caps->supported & BGFX_CAPS_TEXTURE_COMPARE_LEQUAL);
			m_useShadowSampler = m_shadowSamplerSupported;

//			LUT
			float viewlut[16];
			bx::mtxIdentity(viewlut);
			float projlut[16];
			bx::mtxOrtho(projlut, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 500.0f, 0.0, caps->homogeneousDepth);
			bgfx::setViewTransform(3, viewlut, projlut);
			float lut[16];
			bx::mtxSRT(lut, 30.0f, 30.0f, 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, -5.0f, 0.0f);
			bgfx::setTransform(lut);
			bgfx::TextureHandle fbtextures[] =
			{
				bgfx::createTexture2D(
					  516
					, 516
					, false
					, 1
					, bgfx::TextureFormat::RG16F
					, BGFX_TEXTURE_RT 
					),
			};

			LUT = fbtextures[0];
			LUTFBO = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);

			bgfx::setViewRect(3, 0, 0, 516, 516);
			bgfx::setTexture(0, s_textureLUT, LUT, UINT32_MAX);
			bgfx::setViewFrameBuffer(3, LUTFBO);
			lutQuad((float)516, (float)516, true);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
			bgfx::submit(3, shader_LUT);



			m_state[0] = meshStateCreate();
			m_state[0]->m_state = 0;
			m_state[0]->m_program = shader_shadowmap;
			m_state[0]->m_viewId = 1;
			m_state[0]->m_numTextures = 0;

			m_state[1] = meshStateCreate();
			m_state[1]->m_state = 0
				| BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CCW
				| BGFX_STATE_MSAA
				;
			m_state[1]->m_program = shader_plane;
			m_state[1]->m_viewId = 2;
			m_state[1]->m_numTextures = 1;
			m_state[1]->m_textures[0].m_flags = UINT32_MAX;
			m_state[1]->m_textures[0].m_stage = 0;
			m_state[1]->m_textures[0].m_sampler = s_shadowMap;
			m_state[1]->m_textures[0].m_texture = BGFX_INVALID_HANDLE;


			imguiCreate();
		}

		virtual int shutdown() override
		{
			meshUnload(m_mesh);

			// Cleanup.
			bgfx::destroy(m_program);
			bgfx::destroy(s_texturecolor);
			bgfx::destroy(s_texturenormal);
			bgfx::destroy(s_ao_rou_meta);
			cameraDestroy();
			meshStateDestroy(m_state[0]);
			meshStateDestroy(m_state[1]);

			bgfx::destroy(m_vbh);
			bgfx::destroy(m_ibh);
			imguiDestroy();

			// Shutdown bgfx.
			bgfx::shutdown();

			return 0;
		}

		bool update() override
		{
			if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
			{
				imguiBeginFrame(m_mouseState.m_mx
					, m_mouseState.m_my
					, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
					| (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
					| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
					, m_mouseState.m_mz
					, uint16_t(m_width)
					, uint16_t(m_height)
				);

				showExampleDialog(this);

				ImGui::SetNextWindowPos(
					ImVec2(m_width - m_width / 5.0f - 20.0f, 10.0f)
					, ImGuiCond_FirstUseEver
				);
				ImGui::SetNextWindowSize(
					ImVec2(m_width / 5.0f, m_height / 1.3f)
					, ImGuiCond_FirstUseEver
				);

				ImGui::Begin("Lghting Model"
					, NULL
					, 0
				);
				if (ImGui::BeginTabBar("Cubemap", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("Bolonga Cubemap"))
					{
						m_currentLightProbe = LightProbe::Bolonga;
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Kyoto Cubemap"))
					{
						m_currentLightProbe = LightProbe::Kyoto;
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				ImGui::Checkbox("Blinn Phong", &m_settings.blinn_phong);
				ImGui::Checkbox("Pbr", &m_settings.pbr);
				ImGui::Checkbox("Pbr+Ibl", &m_settings.pbr_ibl);
				if (m_settings.blinn_phong == 1 && last_blinn == 0) {
					m_settings.pbr = 0;
					m_settings.pbr_ibl = 0;
					last_blinn = 1;
					last_pbr = 0;
					last_pbr_ibl = 0;
				}
				if (m_settings.pbr == 1 && last_pbr == 0) {
					m_settings.blinn_phong = 0;
					m_settings.pbr_ibl = 0;
					last_blinn = 0;
					last_pbr = 1;
					last_pbr_ibl = 0;
				}
				if (m_settings.pbr_ibl == 1 && last_pbr_ibl == 0) {
					m_settings.pbr = 0;
					m_settings.blinn_phong = 0;
					last_blinn = 0;
					last_pbr = 0;
					last_pbr_ibl = 1;
				}
				if (m_settings.blinn_phong) {
					ImGui::Separator();
					ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
					ImGui::SliderFloat("Diffuse", &diffuse, 0.1f, 1.0f);
					ImGui::SliderFloat("Specular", &specular, 0.1f, 2.0f);
					ImGui::SliderFloat("Specular_index", &st, 0.1f, 32.0f);
				}

				if (m_settings.pbr) {
					ImGui::Separator();
					ImGui::Text("Directional light:");
					ImGui::Checkbox("Direct Diffuse", &m_settings.m_doDiffuse);
					ImGui::SameLine();
					ImGui::Checkbox("Direct Specular", &m_settings.m_doSpecular);
				}
				if (m_settings.pbr_ibl) {
					ImGui::Separator();
					ImGui::Text("Environment light:");
					ImGui::Checkbox("IBL Diffuse", &m_settings.m_doDiffuseIbl);
					ImGui::SameLine();
					ImGui::Checkbox("IBL Specular", &m_settings.m_doSpecularIbl);
					/*ImGui::Text("IBL Specular Elements:");
					ImGui::Checkbox("Prefiltermap", &m_settings.doPrefilterMap);
					ImGui::SameLine();
					ImGui::Checkbox("Brdflut", &m_settings.doBrdfLut);*/
					ImGui::Separator();
					ImGui::Text("Directional light:");
					ImGui::Checkbox("Direct Diffuse", &m_settings.m_doDiffuse);
					ImGui::SameLine();
					ImGui::Checkbox("Direct Specular", &m_settings.m_doSpecular);
				}
				ImGui::Separator();
				ImGui::Text("Extral Setting:");
				ImGui::Checkbox("Shadow Map", &m_settings.shadowmap);
				ImGui::Checkbox("PCF", &m_settings.pcf);

				ImGui::End();
				imguiEndFrame();


				m_uniforms.blinn_phong = float(m_settings.blinn_phong);
				m_uniforms.pbr = float(m_settings.pbr);
				m_uniforms.pbr_ibl = float(m_settings.pbr_ibl);
				m_uniforms.m_bgType = m_settings.m_bgType;

				//lighting control
				m_uniforms.m_doDiffuse = float(m_settings.m_doDiffuse);
				m_uniforms.m_doSpecular = float(m_settings.m_doSpecular);
				m_uniforms.m_doDiffuseIbl = float(m_settings.m_doDiffuseIbl);
				m_uniforms.m_doSpecularIbl = float(m_settings.m_doSpecularIbl);
				m_uniforms.normalMap = float(m_settings.normalMap);
				//specular lighting
				m_uniforms.doPrefilterMap = float(m_settings.doPrefilterMap);
				m_uniforms.doBrdfLut = float(m_settings.doBrdfLut);
				m_uniforms.shadowmap = float(m_settings.shadowmap);
				m_uniforms.pcf = float(m_settings.pcf);

				bx::memCopy(m_uniforms.m_rgbSpec, m_settings.m_rgbSpec, 3 * sizeof(float));
				bx::memCopy(m_uniforms.m_lightCol, m_settings.m_lightCol, 3 * sizeof(float));

				if (bgfx::isValid(m_shadowMapFB))
				{
					bgfx::destroy(m_shadowMapFB);
				}

				float blinn[4] = { ambient, diffuse, specular, st };

				int64_t now = bx::getHPCounter();
				static int64_t last = now;
				const int64_t frameTime = now - last;
				last = now;
				const double freq = double(bx::getHPFrequency());
				const float deltaTimeSec = float(double(frameTime) / freq);
				float time = float((now - m_timeOffset) / freq);
				// Camera.
				const bool mouseOverGui = ImGui::MouseOverArea();
				m_mouse.update(float(m_mouseState.m_mx), float(m_mouseState.m_my), m_mouseState.m_mz, m_width, m_height);
				if (!mouseOverGui)
				{
					if (m_mouseState.m_buttons[entry::MouseButton::Left])
					{
						cameraSetOrbitState(m_mouse.m_dx, m_mouse.m_dy, deltaTimeSec);
					}
					
					else if (0 != m_mouse.m_scroll)
					{
						cameraSetDollyState(float(m_mouse.m_scroll) * 0.05f, deltaTimeSec);
					}
				}
				cameraUpdate(deltaTimeSec);
				bx::Vec3 campos = cameraGetPosition();
				bx::memCopy(m_uniforms.m_cameraPos, &campos.x, 3 * sizeof(float));
				
				bgfx::touch(0);


				bgfx::TextureHandle sdtextures[]
				{
					bgfx::createTexture2D(
						  512
						, 512
						, false
						, 1
						, bgfx::TextureFormat::BGRA8
						, BGFX_TEXTURE_RT
						),
					bgfx::createTexture2D(
						  512
						, 512
						, false
						, 1
						, bgfx::TextureFormat::D16
						, BGFX_TEXTURE_RT_WRITE_ONLY
						),
				};

				/*bgfx::TextureHandle depthtex = bgfx::createTexture2D(
					516
					, 516
					, false
					, 1
					, bgfx::TextureFormat::D16
					, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
				);*/

				shadowMapTexture = sdtextures[0];
				m_shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(sdtextures), sdtextures, true);

				m_state[0]->m_program = shader_shadowmap;
				m_state[0]->m_state = 0;

				m_state[1]->m_program = shader_plane;
				m_state[1]->m_textures[0].m_texture = shadowMapTexture;


				// Setup lights.
				bx::Vec3 lightPos = bx::Vec3(-bx::cos(time), -1.0f, -bx::sin(time));

				bx::memCopy(m_uniforms.lighingPos, &lightPos.x, 3 * sizeof(float));

				// View Transform 0.    render  skybox
				float view[16];
				bx::mtxIdentity(view);

				const bgfx::Caps* caps = bgfx::getCaps();

				float proj[16];
				bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 500.0f, 0.0, caps->homogeneousDepth);
				bgfx::setViewTransform(0, view, proj);

				// View Transform 2   render stone 
				cameraGetViewMtx(view);
				bx::mtxProj(proj, 45.0f, float(m_width) / float(m_height), 0.1f, 500.0f, caps->homogeneousDepth);
				bgfx::setViewTransform(2, view, proj);

				// View Transform 1  render shadow map.
				float lightView[16];
				float lightProj[16];
				
				const bx::Vec3 at = { 0.0f,  0.0f,   0.0f };
				const bx::Vec3 eye = { -lightPos.x, -lightPos.y, -lightPos.z};
				bx::mtxLookAt(lightView, eye, at);
				const float area = 30.0f;
				bx::mtxOrtho(lightProj, -area, area, -area, area, -100.0f, 100.0f, 0.0f, caps->homogeneousDepth);
				
				// Rect space .
				bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewRect(2, 0, 0, uint16_t(m_width), uint16_t(m_height));
				//bgfx::setViewRect(2, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewRect(1, 0, 0, 512, 512);

				//
				bgfx::setViewFrameBuffer(1, m_shadowMapFB);
				bgfx::setViewTransform(1, lightView, lightProj);
				bgfx::setViewClear(1
					, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
					, 0x303030ff, 1.0f, 0
				);



			/*	bgfx::setViewClear(1
					, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
					, 0x303030ff, 1.0f, 0
				);*/
				float mtxShadow[16];
				float lightMtx[16];
				const float sy = caps->originBottomLeft ? 0.5f : -0.5f;
				const float sz = caps->homogeneousDepth ? 0.5f : 1.0f;
				const float tz = caps->homogeneousDepth ? 0.5f : 0.0f;
				const float mtxCrop[16] =
				{
					0.5f, 0.0f, 0.0f, 0.0f,
					0.0f,   sy, 0.0f, 0.0f,
					0.0f, 0.0f, sz,   0.0f,
					0.5f, 0.5f, tz,   1.0f,
				};

				float mtxTmp[16];
				bx::mtxMul(mtxTmp, lightProj, mtxCrop);
				bx::mtxMul(mtxShadow, lightView, mtxTmp);
				

			
				// render env
				const float amount = bx::min(deltaTimeSec / 0.12f, 1.0f);
				m_settings.m_envRotCurr = bx::lerp(m_settings.m_envRotCurr, m_settings.m_envRotDest, amount);

				float mtxEnvView[16];
				cameraGetenvViewMtx(mtxEnvView);
				float mtxEnvRot[16];
				bx::mtxRotateY(mtxEnvRot, m_settings.m_envRotCurr);
				bx::mtxMul(m_uniforms.m_mtx, mtxEnvView, mtxEnvRot); // Used for Skybox.
			
				// Submit view 0.
				bgfx::setTexture(0, s_texturecube, m_lightProbes[m_currentLightProbe].m_tex);
				bgfx::setTexture(1, s_texturecubeirr, m_lightProbes[m_currentLightProbe].m_texIrr);
				bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
				//screenSpaceQuad((float)m_width, (float)m_height, true);
				lutQuad((float)m_width, (float)m_height, true);
				m_uniforms.submit();
				//m_uniforms.submit();
				bgfx::submit(0, m_programSky);

				//render mesh   Submit view 1

				
				float mtx[16];
				bx::mtxSRT(mtx, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
				bgfx::setUniform(Blinn_Phong, blinn);
				//pass1
				bx::mtxMul(lightMtx, mtx, mtxShadow);
				bgfx::setUniform(u_lightMtx, lightMtx);
				//meshSubmit(m_mesh, &m_state[0], 1, mtx);
				bgfx::setState(0);
				meshSubmit(m_mesh, 1, shader_shadowmap, mtx);



				bx::memCopy(m_uniforms.m_mtx, mtxEnvRot, 16 * sizeof(float)); // Used for IBL.
				// Bind textures.
				bgfx::setTexture(0, s_texturecolor,m_texturecolor);
				bgfx::setTexture(1, s_ao_rou_meta, m_ao_rou_meta);
				bgfx::setTexture(2, s_texturenormal, m_ao_rou_meta);
				bgfx::setTexture(3, s_texturecube, m_lightProbes[m_currentLightProbe].m_tex);
				bgfx::setTexture(4, s_texturecubeirr, m_lightProbes[m_currentLightProbe].m_texIrr);
				bgfx::setTexture(5, s_textureLUT, LUT);
				bgfx::setTexture(6, s_shadowMap, shadowMapTexture);
				bgfx::setUniform(u_lightMtx, lightMtx);
				m_uniforms.submit();
				bgfx::setState(0
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_CULL_CCW
					| BGFX_STATE_MSAA
					);
				meshSubmit(m_mesh, 2, m_program, mtx);
			
				//meshSubmit(m_mesh, &m_state[2], 1, mtx);
				//render plane  view 2

				float my_plane[16];
				bx::mtxSRT(my_plane,25.0f, 25.0f, 25.0f, 0.0f, 0.0f, 0.0f, 0.0f, -5.0f, 0.0f);
		
				bx::mtxMul(lightMtx, my_plane, mtxShadow);
				bgfx::setTransform(my_plane);
				bgfx::setUniform(u_lightMtx, lightMtx);
				bgfx::setIndexBuffer(m_ibh);
				bgfx::setVertexBuffer(0, m_vbh);
				bgfx::setState(0);
			
				
				bgfx::submit(1, shader_shadowmap);

				bgfx::setTransform(my_plane);
				bgfx::setUniform(Blinn_Phong, blinn);
				bgfx::setUniform(u_lightMtx, lightMtx);
				bgfx::setTexture(0, s_shadowMap, shadowMapTexture,UINT32_MAX);
				bgfx::setIndexBuffer(m_ibh);
				bgfx::setVertexBuffer(0, m_vbh);
				bgfx::setState(0
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_CULL_CCW
					| BGFX_STATE_MSAA);

				m_uniforms.submit();
				bgfx::submit(2, shader_plane);
			
				// Advance to next frame. Rendering thread will be kicked to
				bgfx::frame();

				return true;
			}

			return false;
		}

		bool last_blinn;
		bool last_pbr;
		bool last_pbr_ibl;
		entry::MouseState m_mouseState;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_debug;
		uint32_t m_reset;

		float ambient;
		float diffuse;
		float specular;
		float st;

		LightProbe m_lightProbes[LightProbe::Count];
		LightProbe::Enum m_currentLightProbe;

		Uniforms m_uniforms;


		bgfx::VertexBufferHandle m_vbh;
		bgfx::IndexBufferHandle m_ibh;

		bgfx::UniformHandle u_mtx;
		bgfx::UniformHandle u_params;
		bgfx::UniformHandle u_flags;
		bgfx::UniformHandle u_camPos;

		bgfx::UniformHandle u_depthScaleOffset;
		bgfx::UniformHandle s_ao_rou_meta;
		bgfx::UniformHandle Blinn_Phong;
		bgfx::UniformHandle s_texturecolor;
		bgfx::UniformHandle s_texturenormal;
		bgfx::UniformHandle s_texturecube;
		bgfx::UniformHandle s_texturecubeirr;
		bgfx::UniformHandle s_shadowMap;
		bgfx::UniformHandle uniform_lightPos;
		bgfx::UniformHandle u_lightMtx;
		bgfx::UniformHandle s_textureLUT;
		bgfx::FrameBufferHandle LUTFBO;
		bgfx::FrameBufferHandle m_shadowMapFB;

		bgfx::TextureHandle shadowMapTexture = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle LUT;
		bgfx::TextureHandle m_ao_rou_meta;
		bgfx::TextureHandle m_texturenormal;
		bgfx::TextureHandle m_texturecolor;
		bgfx::TextureHandle shadow_map_tex;

		Mesh* m_mesh;
		MeshState* m_state[4];
		bgfx::ProgramHandle m_program;
		bgfx::ProgramHandle m_programSky;
		bgfx::ProgramHandle shader_plane;
		bgfx::ProgramHandle shader_shadowmap;
		bgfx::ProgramHandle shader_LUT;
		int64_t m_timeOffset;

		bool m_shadowSamplerSupported;
		bool m_useShadowSampler;

		Settings m_settings;
		Mouse m_mouse;

	};

} // namespace


int _main_(int _argc, char** _argv)
{
	EStar app("Cross-platform rendering ", "", "");
	return entry::runApp(&app, _argc, _argv);
}

