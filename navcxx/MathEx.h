#ifndef MATH_H
#define MATH_H
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <assert.h>
#include <sstream>
#include <stdlib.h>
#include <random>
#include "vector-t.h"

#define PI (float)3.14159265358979323846

namespace Math {

	inline float Rad(float angle) {
		return angle * (PI / 180);
	}

	inline float Deg(float radian) {
		return (radian)* (180 / PI);
	}

	inline float Clamp(float value, float min, float max) {
		if (value < min) {
			value = min;
		} else if (value > max) {
			value = max;
		}
		return value;
	}

	inline float Clamp01(float value) {
		return Clamp(value, 0, 1);
	}

	inline float Min(float a, float b) {
		return a <= b ? a : b;
	}

	inline float Max(float a, float b) {
		return a >= b ? a : b;
	}

	inline int Rand(int min, int max) {
		std::random_device rd;
		std::uniform_int_distribution<int> dist(min, max);
		return dist(rd);
	}

	inline float Rand(float min, float max) {
		std::random_device rd;
		std::uniform_real_distribution<float> dist(min, max);
		return dist(rd);
	}

	bool InsideVector(const Vector3& lhs, const Vector3& rhs, const Vector3& pos);

	bool InsidePoly(std::vector<Vector3>& vertice, const Vector3& pos);

	float SqrDistancePointToSegment(const Vector3& a, const Vector3& u, const Vector3& b, Vector3* p);

	float DistancePointToSegment(const Vector3& start, const Vector3& over, const Vector3& p, Vector3* result);

	float CalcPolyArea(std::vector<const Vector3*>& vertice);

	Vector3 RandomInCircle(const Vector3& center, float radius);

	Vector3 RandomInRectangle(const Vector3& center, float length, float width, float angle);

	Vector3 RandomInTriangle(const Vector3& a, const Vector3& b, const Vector3& c, Vector3& pos);

	Vector3 RandomInPoly(std::vector<const Vector3*>& vertice, float area);


}

#endif