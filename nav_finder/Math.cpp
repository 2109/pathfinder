
#include "Vector3.h"
#include "Math.h"


namespace Math {

	bool InsideVector(const Math::Vector3& lhs, const Math::Vector3& rhs, const Math::Vector3& pos) {
		float lCross = CrossY(lhs, pos);
		float rCross = CrossY(rhs, pos);

		if ((lCross < 0 && rCross > 0) || (lCross == 0 && rCross > 0) || (lCross < 0 && rCross == 0)) {
			return true;
		}
		return false;
	}

	bool InsidePoly(std::vector<Math::Vector3>& vertice, const Math::Vector3& pos) {
		int sign = 0;
		int count = vertice.size();
		for (int i = 0; i < count; ++i) {
			Math::Vector3& pt0 = vertice[i];
			Math::Vector3& pt1 = vertice[(i + 1) % count];
			Math::Vector3 vt0 = pos - pt0;
			Math::Vector3 vt1 = pt1 - pt0;
			float cross = CrossY(vt0, vt1);
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

	float SqrDistancePointToSegment(const Vector3& a, const Vector3& u, const Vector3& b) {
		float t = Dot(b - a, u) / SqrMagnitude(u);
		Vector3 dot = a + u * Clamp(t, 0, 1);
		return DistanceSquared(b, dot);
	}

	float DistancePointToSegment(const Vector3& start, const Vector3& over, const Vector3& p) {
#ifdef USE_AREA
		const float EPS = 1e-6;
		float a, b, c, s;

		a = Math::Distance(p, over);
		if (a < EPS) {
			return 0.0f;
		}

		b = Math::Distance(p, start);
		if (b < EPS) {
			return 0.0f;
		}

		c = Math::Distance(start, over);
		if (c < EPS) {
			return a;
		}

		if (a * a >= b * b + c * c) {
			return b;
		}

		if (b * b >= a * a + c * c) {
			return a;
		}

		s = (a + b + c) / 2;
		s = sqrt(s * (s - a) * (s - b) * (s - c));

		return  2 * s / c;
#else
		Vector3 u = over - start;
		return sqrt(SqrDistancePointToSegment(start, u, p));
#endif
	}

	float CalcPolyArea(std::vector<const Vector3*>& vertice) {
		float area = 0.0f;
		for (int i = 0; i < (int)vertice.size(); ++i) {
			int last = i;
			int next = (i + 1) % vertice.size();
			const Vector3* pt0 = vertice[last];
			const Vector3* pt1 = vertice[next];
			area += pt0->x * pt1->z - pt0->z * pt1->x;
		}
		return fabs(area / 2.0);
	}

	Vector3 RandomInCircle(const Vector3& center, float radius) {
		float angle = Rand(0.0f, 360.0f);
		float rr = Rand(0.0f, radius);

		float dx = sin(Rad(angle)) * rr;
		float dz = cos(Rad(angle)) * rr;

		return Vector3(center.x + dx, 0, center.z + dz);
	}

	Vector3 RandomInRectangle(const Vector3& center, float length, float width, float angle) {
		float dx = Math::Rand(-length / 2, length / 2);
		float dz = Math::Rand(-width / 2, width / 2);

		Vector3 result(center.x + dx, 0, center.z + dz);
		return Rotation(result, center, 360 - angle);
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

		return Vector3(a.x + ab.x * x + ac.x * z, 0, a.z + ab.z * x + ac.z * z);
	}

	Vector3 RandomInPoly(std::vector<const Vector3*>& vertice, float area) {
		if (area <= 0) {
			area = CalcPolyArea(vertice);
		}

		float weight = area * (Rand(0, 10000) / 10000.0f);
		float tmp = 0.0f;
		std::vector<const Vector3*> tri = { NULL, NULL, NULL };
		for (int i = 1; i < vertice.size() - 1; ++i) {
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