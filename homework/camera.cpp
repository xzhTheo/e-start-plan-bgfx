/*
 * Copyright 2013 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/timer.h>
#include <bx/math.h>
#include "camera.h"
#include "entry/entry.h"
#include "entry/cmd.h"
#include "entry/input.h"
#include <bx/allocator.h>



void mtxLookAt_wasd(float* _result, const bx::Vec3& _eye, const bx::Vec3& _at, const bx::Vec3& _up, bx::Handness::Enum _handness)
{
	const bx::Vec3 view = bx::normalize(
		bx::Handness::Right == _handness
		? sub(_eye, _at)
		: sub(_at, _eye)
	);
	const bx::Vec3 uxv = cross(_up, view);
	const bx::Vec3 right = bx::normalize(uxv);
	const bx::Vec3 up = cross(view, right);

	bx::memSet(_result, 0, sizeof(float) * 16);
	_result[0] = right.x;
	_result[1] = up.x;
	_result[2] = view.x;

	_result[4] = right.y;
	_result[5] = up.y;
	_result[6] = view.y;

	_result[8] = right.z;
	_result[9] = up.z;
	_result[10] = view.z;

	_result[12] = -dot(right, _eye);
	_result[13] = -dot(up, _eye);
	_result[14] = -dot(view, _eye);
	_result[15] = 1.0f;
}


int cmdMove(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
	if (_argc > 1)
	{
		if (0 == bx::strCmp(_argv[1], "forward") )
		{
			cameraSetKeyState(CAMERA_KEY_FORWARD, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "left") )
		{
			cameraSetKeyState(CAMERA_KEY_LEFT, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "right") )
		{
			cameraSetKeyState(CAMERA_KEY_RIGHT, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "backward") )
		{
			cameraSetKeyState(CAMERA_KEY_BACKWARD, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "up") )
		{
			cameraSetKeyState(CAMERA_KEY_UP, true);
			return 0;
		}
		else if (0 == bx::strCmp(_argv[1], "down") )
		{
			cameraSetKeyState(CAMERA_KEY_DOWN, true);
			return 0;
		}
	}

	return 1;
}

static void cmd(const void* _userData)
{
	cmdExec( (const char*)_userData);
}

static const InputBinding s_camBindings[] =
{
	{ entry::Key::KeyW,             entry::Modifier::None, 0, cmd, "move forward"  },
	{ entry::Key::GamepadUp,        entry::Modifier::None, 0, cmd, "move forward"  },
	{ entry::Key::KeyA,             entry::Modifier::None, 0, cmd, "move left"     },
	{ entry::Key::GamepadLeft,      entry::Modifier::None, 0, cmd, "move left"     },
	{ entry::Key::KeyS,             entry::Modifier::None, 0, cmd, "move backward" },
	{ entry::Key::GamepadDown,      entry::Modifier::None, 0, cmd, "move backward" },
	{ entry::Key::KeyD,             entry::Modifier::None, 0, cmd, "move right"    },
	{ entry::Key::GamepadRight,     entry::Modifier::None, 0, cmd, "move right"    },
	{ entry::Key::KeyQ,             entry::Modifier::None, 0, cmd, "move down"     },
	{ entry::Key::GamepadShoulderL, entry::Modifier::None, 0, cmd, "move down"     },
	{ entry::Key::KeyE,             entry::Modifier::None, 0, cmd, "move up"       },
	{ entry::Key::GamepadShoulderR, entry::Modifier::None, 0, cmd, "move up"       },

	INPUT_BINDING_END
};

struct Camera
{
	struct MouseCoords
	{
		int32_t m_mx;
		int32_t m_my;
		int32_t m_mz;
	};

	Camera()
	{
		reset();
		entry::MouseState mouseState;
		update(0.0f);

		cmdAdd("move", cmdMove);
		inputAddBindings("camBindings", s_camBindings);
	}

	~Camera()
	{
		cmdRemove("move");
		inputRemoveBindings("camBindings");
	}

	void reset()
	{
		

		m_orbit[0] = 0.0f;
		m_orbit[1] = 0.0f;


		direction = { 0.0f, 0.0f, 1.0f };
		up = { 0.0f, 1.0f, 0.0f };
		right = { -1.0f, 0.0f, 0.0f };

		m_target.curr = { 0.0f, 0.0f, 0.0f };
		m_target.dest = { 0.0f, 0.0f, 0.0f };

		m_pos.curr = { 0.0f, 0.0f, -17.0f };
		m_pos.dest = { 0.0f, 0.0f, -17.0f };

		m_moveSpeed = 15.0f;
		m_keys = 0;
		m_mouseDown = false;
	}

	void setKeyState(uint8_t _key, bool _down)
	{

		m_keys &= ~_key;
		m_keys |= _down ? _key : 0;
	}
	void orbit(float _dx, float _dy, float _deltaTime)
	{
		const float _amount = bx::min(_deltaTime / 0.12f, 1.0f);
		m_orbit[0] += _dx;
		m_orbit[1] += _dy;
		float consume[2];
		consume[0] = m_orbit[0] * _amount;
		consume[1] = m_orbit[1] * _amount;
		m_orbit[0] -= consume[0];
		m_orbit[1] -= consume[1];

		const bx::Vec3 toPos = bx::sub(m_pos.curr, m_target.curr);
		const float toPosLen = bx::length(toPos);
		const float invToPosLen = 1.0f / (toPosLen + bx::kFloatMin);
		const bx::Vec3 toPosNorm = bx::mul(toPos, invToPosLen);

		float ll[2];
		bx::toLatLong(&ll[0], &ll[1], toPosNorm);
		ll[0] += consume[0];
		ll[1] -= consume[1];
		ll[1] = bx::clamp(ll[1], 0.02f, 0.98f);

		const bx::Vec3 tmp = bx::fromLatLong(ll[0], ll[1]);
		const bx::Vec3 diff = bx::mul(bx::sub(tmp, toPosNorm), toPosLen);

		m_pos.curr = bx::add(m_pos.curr, diff);
		m_pos.dest = bx::add(m_pos.dest, diff);
		//m_pos.curr = bx::lerp(m_pos.curr, m_pos.dest, _amount);


		direction = normalize(bx::sub(m_target.curr,m_pos.curr));
		right = normalize(bx::cross(direction, { 0.0f, 1.0f, 0.0f }));
		up = normalize(bx::cross(right, direction));

	}

	void dolly(float _dz,  float _deltaTime)
	{
		const float _amount = bx::min(_deltaTime / 0.12f, 1.0f);
		const float cnear = 1.0f;
		const float cfar = 100.0f;

		const bx::Vec3 toTarget = bx::sub(m_target.curr, m_pos.curr);
		const float toTargetLen = bx::length(toTarget);
		const float invToTargetLen = 1.0f / (toTargetLen + bx::kFloatMin);
		const bx::Vec3 toTargetNorm = bx::mul(toTarget, invToTargetLen);

		float delta = toTargetLen * _dz;
		float newLen = toTargetLen + delta;
		if ((cnear < newLen || _dz < 0.0f)
			&& (newLen < cfar || _dz > 0.0f))
		{
			m_pos.curr = bx::mad(toTargetNorm, delta, m_pos.curr);
		}
		//m_pos.curr = bx::lerp(m_pos.curr, m_pos.dest, _amount);
	}


	void update(float _deltaTime)
	{
	
		/*const float amount = bx::min(_deltaTime / 0.12f, 1.0f);

		consumeOrbit(amount);*/

		// direction = normalize(bx::sub(my_target,my_eye));
		 //right = normalize(bx::cross(direction, up));
		// up = normalize(bx::cross(right, direction));

		
		if (m_keys & CAMERA_KEY_FORWARD)
		{
			m_pos.curr = bx::mad(direction, _deltaTime * m_moveSpeed, m_pos.curr);

			setKeyState(CAMERA_KEY_FORWARD, false);
		}

		if (m_keys & CAMERA_KEY_BACKWARD)
		{
			m_pos.curr = bx::mad(direction, -_deltaTime * m_moveSpeed, m_pos.curr);

			setKeyState(CAMERA_KEY_BACKWARD, false);
		}

		if (m_keys & CAMERA_KEY_LEFT)
		{
			m_pos.curr = bx::mad(right, _deltaTime * m_moveSpeed, m_pos.curr);
			//m_target.curr = bx::mad(right, -_deltaTime * m_moveSpeed, m_target.curr);
			setKeyState(CAMERA_KEY_LEFT, false);
		}

		if (m_keys & CAMERA_KEY_RIGHT)
		{
			m_pos.curr = bx::mad(right, -_deltaTime * m_moveSpeed, m_pos.curr);
			//m_target.curr = bx::mad(right, -_deltaTime * m_moveSpeed, m_target.curr);
			setKeyState(CAMERA_KEY_RIGHT, false);
		}

		if (m_keys & CAMERA_KEY_UP)
		{
			m_pos.curr = bx::mad(up, _deltaTime * m_moveSpeed, m_pos.curr);
	
			setKeyState(CAMERA_KEY_UP, false);
		}

		if (m_keys & CAMERA_KEY_DOWN)
		{
			m_pos.curr = bx::mad(up, -_deltaTime * m_moveSpeed, m_pos.curr);
			
			setKeyState(CAMERA_KEY_DOWN, false);
		}


		//m_target.curr = bx::add(m_pos.curr, direction);
		//my_eye = bx::sub(my_target, direction);
		//up = bx::cross(right, direction);
	}

	void getViewMtx(float* _viewMtx)
	{
		bx::mtxLookAt(_viewMtx, m_pos.curr, add(m_pos.curr , direction));
		//bx::mtxLookAt(_viewMtx, m_pos.curr, m_target.curr);
	}

	void setPosition(const bx::Vec3& _pos)
	{
		m_pos.curr = _pos;
	}
	void envViewMtx(float* _mtx)
	{
		const bx::Vec3 toTarget = bx::sub(m_target.curr, m_pos.curr);
		const float toTargetLen = bx::length(toTarget);
		const float invToTargetLen = 1.0f / (toTargetLen + bx::kFloatMin);
		const bx::Vec3 toTargetNorm = bx::mul(toTarget, invToTargetLen);

		const bx::Vec3 right = bx::normalize(bx::cross({ 0.0f, 1.0f, 0.0f }, toTargetNorm));
		const bx::Vec3 up = bx::normalize(bx::cross(toTargetNorm, right));

		_mtx[0] = right.x;
		_mtx[1] = right.y;
		_mtx[2] = right.z;
		_mtx[3] = 0.0f;
		_mtx[4] = up.x;
		_mtx[5] = up.y;
		_mtx[6] = up.z;
		_mtx[7] = 0.0f;
		_mtx[8] = toTargetNorm.x;
		_mtx[9] = toTargetNorm.y;
		_mtx[10] = toTargetNorm.z;
		_mtx[11] = 0.0f;
		_mtx[12] = 0.0f;
		_mtx[13] = 0.0f;
		_mtx[14] = 0.0f;
		_mtx[15] = 1.0f;
	}


	float m_orbit[2];

	struct Interp3f
	{
		bx::Vec3 curr = bx::init::None;
		bx::Vec3 dest = bx::init::None;
	};

	Interp3f m_target;
	Interp3f m_pos;

	bx::Vec3 direction = bx::init::Zero;
	bx::Vec3 up = bx::init::Zero;
	bx::Vec3 right = bx::init::Zero;
	float m_moveSpeed;

	uint8_t m_keys;
	bool m_mouseDown;
};

static Camera* s_camera = NULL;


void cameraCreate()
{
	s_camera = BX_NEW(entry::getAllocator(), Camera);
}

void cameraDestroy()
{
	BX_DELETE(entry::getAllocator(), s_camera);
	s_camera = NULL;
}

void cameraSetDollyState(float _dz, float _deltaTime) {
	s_camera->dolly(_dz,  _deltaTime);

}
void cameraSetOrbitState(float _dx, float _dy, float _deltaTime) {
	s_camera->orbit(_dx, _dy,_deltaTime);
}


void cameraSetPosition(const bx::Vec3& _pos)
{
	s_camera->setPosition(_pos);
}
void cameraGetenvViewMtx(float* _mtx) {
	s_camera->envViewMtx(_mtx);
}


void cameraSetKeyState(uint8_t _key, bool _down)
{
	s_camera->setKeyState(_key, _down);
}

void cameraGetViewMtx(float* _viewMtx)
{
	s_camera->getViewMtx(_viewMtx);
}

bx::Vec3 cameraGetPosition()
{
	return s_camera->m_pos.curr;
}

bx::Vec3 cameraGetAt()
{
	return s_camera->m_target.curr;
}

void cameraUpdate(float _deltaTime)
{
	s_camera->update(_deltaTime);
}
