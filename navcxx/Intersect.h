#ifndef INTERSECT_H
#define INTERSECT_H
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <assert.h>
#include "vector-t.h"
// #include "Vector2.h"
// #include "Vector3.h"

namespace Math {
	bool IsCapsuleCircleIntersect(const Vector<float, 2>& x0, const Vector<float, 2>& x1, float cr, const Vector<float, 2>& center, float r);

	bool IsCircleCircleIntersect(const Vector2& src, float l, const Vector2& center, float r);

	bool IsSegmentCircleIntersect(const Vector2& x0, const Vector2& x1, const Vector2& center, float r);

	bool IsRectangleCirecleIntersect(const Vector2& src, float l, float w, float angle, const Vector2& center, float r);

	bool IsSectorCircleIntersect(const Vector2& src, float angle, float degree, float l, const Vector2& center, float r);

	bool InFrontOf(const Vector2& a, float angle, const Vector2& dot);

	bool InsideCircle(const Vector2& a, float l, const Vector2& center, float r);

	bool InsideSector(const Vector2& a, float angle, float degree, float l, const Vector2& center, float r);

	bool InsideRectangle(const Vector2& a, float angle, float length, float width, const Vector2& center, float r);

	Vector2 RandomInCircle(const Vector2& center, float radius);

	Vector2 RandomInRectangle(const Vector2& center, float length, float width, float angle);

	Vector3 IntersectLineWithPlane(const Vector3& point, const Vector3& direct, const Vector3& planeNormal, const Vector3& planePoint);

	bool SegmentIntersect(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d);

	bool Intersect(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d);

	void GetIntersectPoint(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d, Vector3& result);
}


#endif
