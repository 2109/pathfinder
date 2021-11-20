#ifndef PLANE_H
#define PLANE_H

#include "vector-t.h"

namespace Math {
	class Plane {
	public:
		enum PLANE_SIDE {
			eINSIDE,
			eBACK,
			eFRONT
		};

		Plane() : normal_(), distance_(0) {

		}

		Plane(float a, float b, float c, float d) {
			Set(a, b, c, d);
		}

		Plane(const Vector3& vt, float distance) {
			normal_ = vt;
			Normalize(normal_);
			distance_ = distance;
		}

		Plane(const Vector3& vt, const Vector3& pt) {
			Set(vt, pt);
		}

		inline void Set(float a, float b, float c, float d);

		inline void Set(const Vector3& a, const Vector3& b, const Vector3& c);

		inline void Set(const Vector3& vt, const Vector3& pt);

		inline float GetDistance(const Vector3& pt);

		inline bool SameSide(const Vector3& pt0, const Vector3& pt1);

		inline PLANE_SIDE GetSide(const Vector3& pt);

		inline bool LineHit(const Vector3& v0, const Vector3& v1, Vector3& ret);

	private:
		Vector3 normal_;
		float distance_;
	};

	inline Vector3 CalcNormalFromTriangle(const Vector3& a, const Vector3& b, const Vector3& c) {
		return Cross(b - a, c - a);
	}

	inline void Plane::Set(float a, float b, float c, float d) {
		normal_.v[0] = a;
		normal_.v[1] = b;
		normal_.v[2] = c;
		distance_ = d;
	}

	inline void Plane::Set(const Vector3& a, const Vector3& b, const Vector3& c) {
		normal_ = CalcNormalFromTriangle(a, b, c);
		Normalize(normal_);
		distance_ = -Dot(normal_, a);
	}

	inline void Plane::Set(const Vector3& vt, const Vector3& pt) {
		normal_ = vt;
		Normalize(normal_);
		distance_ = -Dot(normal_, pt);
	}

	inline float Plane::GetDistance(const Vector3& pt) {
		return Dot(normal_, pt) + distance_;
	}

	inline bool Plane::SameSide(const Vector3& pt0, const Vector3& pt1) {
		float d0 = GetDistance(pt0);
		float d1 = GetDistance(pt1);
		if (d0 > 0.0f && d1 > 0.0f)
			return true;
		else if (d0 <= 0.0f && d1 <= 0.0f)
			return true;
		else
			return false;
	}

	inline Plane::PLANE_SIDE Plane::GetSide(const Vector3& pt) {
		float dist = Dot(normal_, pt) + distance_;
		if (dist == 0) {
			eINSIDE;
		} else if (dist < 0) {
			return eBACK;
		} else {
			return eFRONT;
		}
	}

	inline bool Plane::LineHit(const Vector3& v0, const Vector3& v1, Vector3& ret) {
		Vector3 dir = v1 - v0;
		float d = GetDistance(v0);
		float f = Dot(normal_, dir);
		if (fabs(f) < Math::Epsilon) return false;
		ret = v0 - (d / f) * dir;
		return true;
	}

}


#endif