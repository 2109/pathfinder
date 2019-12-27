
#ifndef VECTOR2_H_
#define VECTOR2_H_
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <assert.h>
#include <sstream>
#include "MathEx.h"

namespace Math {
	class Vector2 {
	public:
		float x;
		float y;

		Vector2();
		explicit Vector2(float x, float y);

		Vector2		operator+(const Vector2& vector) const;
		Vector2		operator-(const Vector2& vector) const;
		Vector2		operator*(const Vector2& vector) const;
		Vector2		operator/(const Vector2& vector) const;
		Vector2&	operator+=(const Vector2& vector);
		Vector2&	operator-=(const Vector2& vector);
		Vector2&	operator*=(const Vector2& vector);
		Vector2&	operator/=(const Vector2& vector);

		Vector2		operator-() const;
		Vector2		operator+(const float num) const;
		Vector2		operator-(const float num) const;
		Vector2		operator*(const float num) const;
		Vector2		operator/(const float num) const;
		Vector2&	operator+=(const float num);
		Vector2&	operator-=(const float num);
		Vector2&	operator*=(const float num);
		Vector2&	operator/=(const float num);

		bool		operator==(const Vector2& vector) const;
		bool		operator!=(const Vector2& vector) const;

		static const Vector2 Zero;
		static const Vector2 One;
		static const Vector2 Up;
		static const Vector2 Down;
		static const Vector2 Left;
		static const Vector2 Right;

		friend std::ostream& operator<< (std::ostream& ofs, const Vector2& vector);
	};

	inline Vector2::Vector2() : x(0), y(0) {}
	inline Vector2::Vector2(float x, float y) : x(x), y(y) {}

	inline Vector2 Vector2::operator+(const Vector2& vector) const {
		return Vector2(x + vector.x, y + vector.y);
	}

	inline Vector2 Vector2::operator-(const Vector2& vector) const {
		return Vector2(x - vector.x, y - vector.y);
	}

	inline Vector2 Vector2::operator*(const Vector2& vector) const {
		return Vector2(x * vector.x, y * vector.y);
	}

	inline Vector2 Vector2::operator/(const Vector2& vector) const {
		return Vector2(x / vector.x, y / vector.y);
	}

	inline Vector2& Vector2::operator+=(const Vector2& vector) {
		x += vector.x;
		y += vector.y;
		return *this;
	}

	inline Vector2& Vector2::operator-=(const Vector2& vector) {
		x -= vector.x;
		y -= vector.y;
		return *this;
	}

	inline Vector2& Vector2::operator*=(const Vector2& vector) {
		x *= vector.x;
		y *= vector.y;
		return *this;
	}

	inline Vector2& Vector2::operator/=(const Vector2& vector) {
		x /= vector.x;
		y /= vector.y;
		return *this;
	}

	inline Vector2 Vector2::operator-() const {
		return Vector2(-x, -y);
	}

	inline Vector2 Vector2::operator-(const float num) const {
		return Vector2(x - num, y - num);
	}

	inline Vector2 Vector2::operator+(const float num) const {
		return Vector2(x + num, y + num);
	}

	inline Vector2 Vector2::operator*(const float num) const {
		return Vector2(x * num, y * num);
	}

	inline Vector2 Vector2::operator/(const float num) const {
		return Vector2(x / num, y / num);
	}

	inline Vector2& Vector2::operator+=(const float num) {
		x += num;
		y += num;
		return *this;
	}

	inline Vector2& Vector2::operator-=(const float num) {
		x -= num;
		y -= num;
		return *this;
	}

	inline Vector2& Vector2::operator*=(const float num) {
		x *= num;
		y *= num;
		return *this;
	}

	inline Vector2& Vector2::operator/=(const float num) {
		x /= num;
		y /= num;
		return *this;
	}

	inline bool Vector2::operator==(const Vector2& vector) const {
		return x == vector.x && y == vector.y;
	}

	inline bool Vector2::operator!=(const Vector2& vector) const {
		return x != vector.x || y != vector.y;
	}

	inline const Vector2 operator*(float lhs, const Vector2& rhs) {
		Vector2 result;
		result.x = rhs.x * lhs;
		result.y = rhs.y * lhs;
		return result;
	}

	inline void Zero(Vector2 &lhs) {
		lhs.x = 0.0f;
		lhs.y = 0.0f;
	}

	inline const bool IsZero(const Vector2& lhs) {
		return lhs.x == 0 && lhs.y == 0;
	}

	inline const float Dot(const Vector2& lhs, const Vector2& rhs) {
		return lhs.x*rhs.x + lhs.y*rhs.y;
	}

	inline const float SqrMagnitude(const Vector2& rhs) {
		return Dot(rhs, rhs);
	}

	inline const float Magnitude(const Vector2& rhs) {
		return sqrt(SqrMagnitude(rhs));
	}

	inline Vector2 Normalized(const Vector2& lhs) {
		return (1.f / (Magnitude(lhs))) * lhs;
	}

	inline void ToNormalize(Vector2& lhs) {
		lhs * (1.f / (Magnitude(lhs) + FLT_MIN));
	}

	inline float Cross(const Vector2& lhs, const Vector2& rhs) {
		return (lhs.x * rhs.y - lhs.y * rhs.x);
	}

	inline const int Sign(const Vector2& lhs, const Vector2& rhs) {
		if (lhs.y * rhs.x > lhs.x * rhs.y) {
			return -1;
		} else {
			return 1;
		}
	}

	inline void ClampMagnitude(Vector2 &rhs, float max) {
		if (Magnitude(rhs) > max) {
			ToNormalize(rhs);
			rhs = rhs* max;
		}
	}

	inline Vector2 Neg(const Vector2& rhs) {
		return Vector2(-rhs.x, -rhs.y);
	}

	inline Vector2 Max(const Vector2& lhs, const Vector2& rhs) {
		return Vector2(Math::Max(lhs.x, rhs.x), Math::Max(lhs.y, rhs.y));
	}

	inline Vector2 Min(const Vector2& lhs, const Vector2& rhs) {
		return Vector2(Math::Min(lhs.x, rhs.x), Math::Min(lhs.y, rhs.y));
	}

	inline Vector2 Abs(const Vector2& vt) {
		return Vector2(vt.x >= 0 ? vt.x : -vt.x, vt.y >= 0 ? vt.y : -vt.y);
	}

	inline float Distance(const Vector2& lhs, const Vector2& rhs) {
		float dx = rhs.x - lhs.x;
		float dy = rhs.y - lhs.y;
		return sqrt(dx * dx + dy * dy);
	}

	inline float DistanceSquared(const Vector2& lhs, const Vector2& rhs) {
		float dx = rhs.x - lhs.x;
		float dy = rhs.y - lhs.y;
		return dx * dx + dy * dy;
	}

	inline Vector2 Reflect(const Vector2& direction, const Vector2& normal) {
		return -2.0f * Dot(normal, direction) * normal + direction;
	}

	inline Vector2 Lerp(const Vector2& vt1, const Vector2& vt2, float num) {
		return (vt1 + ((vt2 - vt1) * Math::Clamp(num, 0, 1)));
	}

	inline float Angle(const Vector2& lhs, const Vector2& rhs) {
		float scalar = sqrt(SqrMagnitude(lhs) * SqrMagnitude(rhs));
		return Rad(acos(Dot(lhs, rhs) / scalar));
	}

	inline Vector2 Rotation(const Vector2& from, const Vector2& center, float angle) {
		float r = Rad(angle);
		float si = sin(r);
		float co = cos(r);

		float dx = from.x - center.x;
		float dy = from.y - center.y;
		float rx = dx * co - dy * si + center.x;
		float ry = dx * si + dy * co + center.y;
		return Vector2(rx, ry);
	}

	inline Vector2 MoveTorward(const Vector2& from, const Vector2& to, float pass) {
		float dt = Distance(from, to);
		if (dt <= 0.000001f) {
			return from;
		}
		return Lerp(from, to, pass / dt);
	}

	inline Vector2 MoveWithAngle(Vector2& from, float angle, float pass) {
		float radian = Rad(angle);
		float x = cos(radian) * pass + from.x;
		float y = sin(radian) * pass + from.y;
		return Vector2(x, y);
	}

	inline std::ostream& operator<< (std::ostream& os, const Vector2& vector) {
		std::stringstream stream;
		stream << "x: " << vector.x << ", y: " << vector.y;
		os.write(const_cast<char*>(stream.str().c_str()), static_cast<std::streamsize>(stream.str().size() * sizeof(char)));
		return os;
	}

}



#endif /* VECTOR2_H_ */
