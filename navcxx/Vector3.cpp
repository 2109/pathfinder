
#include "Vector3.h"

namespace Math {
	const Vector3 Vector3::Zero(0, 0, 0);
	const Vector3 Vector3::Left(-1.f, 0.f, 0.f);
	const Vector3 Vector3::Right(1.f, 0.f, 0.f);
	const Vector3 Vector3::Up(0.f, 1.f, 0.f);
	const Vector3 Vector3::Down(0.f, -1.f, 0.f);
	const Vector3 Vector3::Forward(0.f, 0.f, 1.f);
	const Vector3 Vector3::Backward(0.f, 0.f, -1.f);
}