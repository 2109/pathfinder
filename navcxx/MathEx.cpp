#include "mathex.h"

namespace Math {

	bool InsideVector(const Vector3& lhs, const Vector3& rhs, const Vector3& pos) {
		float lc = Cross_Y(lhs, pos);
		float rc = Cross_Y(rhs, pos);

		if ((lc < 0 && rc > 0) || (lc == 0 && rc > 0) || (lc < 0 && rc == 0)) {
			return true;
		}
		return false;
	}

	bool InsidePoly(std::vector<Vector3>& vertice, const Vector3& pos) {
		int sign = 0;
		int count = vertice.size();
		for (int i = 0; i < count; ++i) {
			Vector3& pt0 = vertice[i];
			Vector3& pt1 = vertice[(i + 1) % count];
			Vector3 vt0 = pos - pt0;
			Vector3 vt1 = pt1 - pt0;
			float cross = Cross_Y(vt0, vt1);
			if (cross == 0) {
				continue;
			}
			if (sign == 0) {
				sign = cross > 0 ? 1 : -1;
			} else {
				if (sign == 1 && cross < 0) {
					return false;
				} else if (sign == -1 && cross > 0) {
					return false;
				}
			}
		}
		return true;
	}

	float SqrDistancePointToSegment(const Vector3& a, const Vector3& u, const Vector3& b, Vector3* p) {
		float t = Dot(b - a, u) / LengthSquared(u);
		Vector3 dot = a + u * Clamp(t, 0, 1);
		if (p) {
			*p = dot;
		}
		return DistanceSquared(b, dot);
	}

	float DistancePointToSegment(const Vector3& start, const Vector3& over, const Vector3& p, Vector3* result) {

		Vector3 u = over - start;
		return sqrt(SqrDistancePointToSegment(start, u, p, result));
	}

	float CalcPolyArea(std::vector<const Vector3*>& vertice) {
		float area = 0.0f;
		for (int i = 0; i < (int)vertice.size(); ++i) {
			int last = i;
			int next = (i + 1) % vertice.size();
			const Vector3* pt0 = vertice[last];
			const Vector3* pt1 = vertice[next];
			area += pt0->x() * pt1->z() - pt0->z() * pt1->x();
		}
		return fabs(area / 2.0);
	}

	Vector3 RandomInCircle(const Vector3& center, float radius) {
		float angle = Rand(0.0f, 360.0f);
		float rr = Rand(0.0f, radius);

		float dx = sin(Rad(angle)) * rr;
		float dz = cos(Rad(angle)) * rr;

		return Vector3(center[0] + dx, 0.0f, center[2] + dz);
	}

	Vector3 RandomInRectangle(const Vector3& center, float length, float width, float angle) {
		float dx = Math::Rand(-length / 2, length / 2);
		float dz = Math::Rand(-width / 2, width / 2);

		Vector3 result(center[0] + dx, 0.0f, center[2] + dz);
		Rotate(result, center, 360 - angle);
		return result;
	}

	Vector3 RandomInTriangle(const Vector3& a, const Vector3& b, const Vector3& c) {
		Vector3 ab = b - a;
		Vector3 ac = c - a;

		float x = Rand(0, 10000) / 10000.0f;
		float z = Rand(0, 10000) / 10000.0f;

		if (x + z > 1) {
			x = 1 - x;
			z = 1 - z;
		}

		return Vector3(a[0] + ab[0] * x + ac[0] * z, 0.0f, a[2] + ab[2] * x + ac[2] * z);
	}

	Vector3 RandomInPoly(std::vector<const Vector3*>& vertice, float area) {
		if (area <= 0) {
			area = CalcPolyArea(vertice);
		}

		float weight = area * (Rand(0, 10000) / 10000.0f);
		float tmp = 0.0f;
		std::vector<const Vector3*> tri = { NULL, NULL, NULL };
		for (unsigned int i = 1; i < vertice.size() - 1; ++i) {
			tri[0] = vertice[0];
			tri[1] = vertice[i];
			tri[2] = vertice[i + 1];
			float triArea = CalcPolyArea(tri);
			tmp += triArea;
			if (weight <= tmp) {
				break;
			}
		}

		return RandomInTriangle(*(tri[0]), *(tri[1]), *(tri[2]));
	}
}