#ifndef VECTOR3_H_
#define VECTOR3_H_


#include "Math.h"
#include "Vector2.h"
namespace Math {
	class Vector3 {
	public:
		float x;
		float y;
		float z;

		Vector3();
		Vector3(Vector2 vt2);
		explicit Vector3(float x, float y, float z);

		Vector3&	operator=(const Vector3& vector);
		Vector3		operator+(const Vector3& vector) const;
		Vector3		operator-(const Vector3& vector) const;
		Vector3		operator*(const Vector3& vector) const;
		Vector3		operator/(const Vector3& vector) const;
		Vector3&	operator+=(const Vector3& vector);
		Vector3&	operator-=(const Vector3& vector);
		Vector3&	operator*=(const Vector3& vector);
		Vector3&	operator/=(const Vector3& vector);

		Vector3		operator-() const;
		Vector3		operator+(float num) const;
		Vector3		operator-(float num) const;
		Vector3		operator*(float num) const;
		Vector3		operator/(float num) const;
		Vector3&	operator+=(float num);
		Vector3&	operator-=(float num);
		Vector3&	operator*=(float num);
		Vector3&	operator/=(float num);

		bool		operator==(const Vector3& vector) const;
		bool		operator!=(const Vector3& vector) const;

		static const Vector3 Zero;
		static const Vector3 Left;
		static const Vector3 Right;
		static const Vector3 Up;
		static const Vector3 Down;
		static const Vector3 Forward;
		static const Vector3 Backward;

		friend std::ostream& operator<< (std::ostream& ofs, const Vector3& vector);
	};

	inline Vector3::Vector3() : x(0), y(0), z(0) {}
	inline Vector3::Vector3(Vector2 vt) : x(vt.x), y(0), z(vt.y) {}
	inline Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	inline Vector3& Vector3::operator=(const Vector3& vector) {
		x = vector.x;
		y = vector.y;
		z = vector.z;
		return *this;
	}

	inline Vector3 Vector3::operator+(const Vector3& vector) const {
		return Vector3(x + vector.x, y + vector.y, z + vector.z);
	}

	inline Vector3 Vector3::operator-(const Vector3& vector) const {
		return Vector3(x - vector.x, y - vector.y, z - vector.z);
	}

	inline Vector3 Vector3::operator*(const Vector3& vector) const {
		return Vector3(x * vector.x, y * vector.y, z * vector.z);
	}

	inline Vector3 Vector3::operator/(const Vector3& vector) const {
		return Vector3(x / vector.x, y / vector.y, z / vector.z);
	}

	inline Vector3& Vector3::operator+=(const Vector3& vector) {
		x += vector.x;
		y += vector.y;
		z += vector.z;
		return *this;
	}

	inline Vector3& Vector3::operator-=(const Vector3& vector) {
		x -= vector.x;
		y -= vector.y;
		z -= vector.z;
		return *this;
	}

	inline Vector3& Vector3::operator*=(const Vector3& vector) {
		x *= vector.x;
		y *= vector.y;
		z *= vector.z;
		return *this;
	}

	inline Vector3& Vector3::operator/=(const Vector3& vector) {
		x /= vector.x;
		y /= vector.y;
		z /= vector.z;
		return *this;
	}

	inline Vector3 Vector3::operator-() const {
		return Vector3(-x, -y, -z);
	}

	inline Vector3 Vector3::operator-(float num) const {
		return Vector3(x - num, y - num, z - num);
	}

	inline Vector3 Vector3::operator+(float num) const {
		return Vector3(x + num, y + num, z + num);
	}

	inline Vector3 Vector3::operator*(float num) const {
		return Vector3(x * num, y * num, z * num);
	}

	inline Vector3 Vector3::operator/(float num) const {
		return Vector3(x / num, y / num, z / num);
	}

	inline Vector3& Vector3::operator+=(float num) {
		x += num;
		y += num;
		z += num;
		return *this;
	}

	inline Vector3& Vector3::operator-=(float num) {
		x -= num;
		y -= num;
		z -= num;
		return *this;
	}

	inline Vector3& Vector3::operator*=(float num) {
		x *= num;
		y *= num;
		z *= num;
		return *this;
	}

	inline Vector3& Vector3::operator/=(float num) {
		x /= num;
		y /= num;
		z /= num;
		return *this;
	}

	inline bool Vector3::operator==(const Vector3& vector) const {
		return x == vector.x && y == vector.y && z == vector.z;
	}

	inline bool Vector3::operator!=(const Vector3& vector) const {
		return x != vector.x || y != vector.y || z != vector.z;
	}

	inline const Vector3 operator*(float lhs, const Vector3& rhs) {
		Vector3 result;
		result.x = rhs.x * lhs;
		result.y = rhs.y * lhs;
		result.z = rhs.z * lhs;
		return result;
	}

	inline void Zero(Vector3& vt) {
		vt.x = 0.0f;
		vt.y = 0.0f;
		vt.z = 0.0f;
	}

	inline const bool IsZero(const Vector3& vt) {
		return vt.x == 0 && vt.y == 0 && vt.z == 0;
	}

	inline float Dot(const Vector3& lhs, const Vector3& rhs) {
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	inline const float SqrMagnitude(const Vector3& rhs) {
		return Dot(rhs, rhs);
	}

	inline const float Magnitude(const Vector3& rhs) {
		return sqrt(SqrMagnitude(rhs));
	}

	inline Vector3 Normalize(Vector3 const &rhs) {
		return rhs * (1.f / (Magnitude(rhs) + FLT_MIN));
	}

	inline void ToNormalize(Vector3& lhs) {
		lhs * (1.f / (Magnitude(lhs) + FLT_MIN));
	}

	inline Vector3 Cross(const Vector3& lhs, const Vector3& rhs) {
		return Vector3(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
	}

	inline float CrossY(const Vector3& lhs, const Vector3& rhs) {
		return lhs.z * rhs.x - lhs.x * rhs.z;
	}

	inline const int Sign(const Vector3& lhs, const Vector3& rhs) {
		if (rhs.y*lhs.x > rhs.x*lhs.y) {
			return -1;
		} else {
			return 1;
		}
	}

	inline void ClampMagnitude(Vector3& vector, float max) {
		if (Magnitude(vector) > max) {
			ToNormalize(vector);
			vector *= max;
		}
	}

	inline Vector3 Neg(const Vector3 &vector) {
		return Vector3(-vector.x, -vector.y, -vector.z);
	}

	inline Vector3 Max(const Vector3& lhs, const Vector3& rhs) {
		return Vector3(Math::Max(lhs.x, rhs.x), Math::Max(lhs.y, rhs.y), Math::Max(lhs.z, rhs.z));
	}

	inline Vector3 Min(const Vector3& lhs, const Vector3& rhs) {
		return Vector3(Math::Min(lhs.x, rhs.x), Math::Min(lhs.y, rhs.y), Math::Min(lhs.z, rhs.z));
	}

	inline Vector3 Abs(const Vector3& vt) {
		return Vector3(vt.x >= 0 ? vt.x : -vt.x, vt.y >= 0 ? vt.y : -vt.y, vt.z >= 0 ? vt.z : -vt.z);
	}

	inline float Distance(const Vector3 &lhs, const Vector3 &rhs) {
		float dx = lhs.x - rhs.x;
		float dy = lhs.y - rhs.y;
		float dz = lhs.z - rhs.z;
		return sqrtf(dy * dy + dx * dx + dz * dz);
	}

	inline float DistanceSquared(Vector3 const&lhs, Vector3 const&rhs) {
		float dx = lhs.x - rhs.x;
		float dy = lhs.y - rhs.y;
		float dz = lhs.z - rhs.z;
		return dy * dy + dx * dx + dz * dz;
	}

	inline Vector3 Reflect(const Vector3& direction, const Vector3& normal) {
		return -2.0f * Dot(normal, direction) * normal + direction;
	}

	inline Vector3 Lerp(const Vector3& from, const Vector3& to, float t) {
		return (from + ((to - from) * Math::Clamp(t, 0, 1)));
	}

	inline Vector3 Slerp(const Vector3& from, const Vector3& to, float t) {
		float dot = Dot(from, to);
		float theta = acos(dot) * Math::Clamp(t, 0, 1);
		Vector3 relativeVt = to - from * dot;
		ToNormalize(relativeVt);
		return ((from * cos(theta)) + (relativeVt * sin(theta)));
	}

	inline float Angle(const Vector3& lhs, const Vector3& rhs) {
		float scalar = sqrt(SqrMagnitude(lhs) * SqrMagnitude(rhs));
		assert(scalar != 0.0f);
		return Rad(acos(Dot(lhs, rhs) / scalar));
	}

	inline Vector3 Rotation(const Vector3& from, const Vector3& axis, float angle) {
		Vector3 o = axis * Dot(axis, from);
		Vector3 _x = from - o;
		Vector3 _y = Cross(axis, from);
		return o + _x * cos(angle) + _y * sin(angle);
	}

	inline Vector3 MoveTorward(Vector3 const& from, Vector3 const& to, float pass) {
		float dt = Distance(from, to);
		if (dt <= 0.000001f) {
			return from;
		}
		return Lerp(from, to, pass / dt);
	}

	inline std::ostream& operator<< (std::ostream& os, const Vector3& vector) {
		std::stringstream stream;
		stream << "x: " << vector.x << ", y: " << vector.y << ", z: " << vector.z;
		os.write(const_cast<char*>(stream.str().c_str()), static_cast<std::streamsize>(stream.str().size() * sizeof(char)));
		return os;
	}
	//-------------------------------------------------------------------

}


#endif /* VECTOR3_H_ */
