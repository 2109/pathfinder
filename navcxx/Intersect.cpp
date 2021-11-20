
#include "Intersect.h"
#include "MathEx.h"


namespace Math {
	bool IsCapsuleCircleIntersect(const Vector2& a, const Vector2& b, float cr, const Vector2& center, float r) {
		Vector2 u = b - a;
		return SqrDistancePointToSegment(Vector3(a), Vector3(u), Vector3(center), NULL) <= (cr + r);
	}

	bool IsCircleCircleIntersect(const Vector2& src, float l, const Vector2& center, float r) {
		Vector2 d = src - center;
		float range = l + r;
		if (fabs(d[0]) <= range && fabs(d[1]) <= range) {
			return LengthSquared(d) <= (range * range);
		}
		return false;
	}

	bool IsSegmentCircleIntersect(const Vector2& x0, const Vector2& x1, const Vector2& center, float r) {
		Vector2 u = x1 - x0;
		return SqrDistancePointToSegment(x0, u, center, NULL) <= r;
	}

	bool IsRectangleCirecleIntersect(const Vector2& a, float l, float w, float angle, const Vector2& circleCenter, float r) {
		float radian = Math::Rad(angle);

		Vector2 reactangleCenter(a[0] + (cos(radian) * (l / 2)), a[1] + (sin(radian) * (l / 2)));

		Vector2 relativeCenter = circleCenter - reactangleCenter;

		Rotate(relativeCenter, Vector2_Zero, 360 - angle);

		Vector2 h(l / 2, w / 2);

		Vector2 v = Abs(relativeCenter);
		Vector2 u = Max(v - h, Vector2_Zero);
		return LengthSquared(u) <= r * r;
	}

	bool IsSectorCircleIntersect(const Vector2& a, float angle, float degree, float l, const Vector2& c, float r) {
		Vector2 d = c - a;
		float range = l + r;

		float magnitude2 = LengthSquared(d);
		if (magnitude2 > range * range) {
			return false;
		}

		float radian = Math::Rad(angle);

		Vector2 u(cos(radian), sin(radian));

		float px = Dot(d, u);
		float py = fabs(Dot(d, Vector2(-u[1], u[0])));

		float theta = Math::Rad(degree / 2);
		if (px > sqrt(magnitude2) * cos(theta)) {
			return true;
		}

		Vector2 q(l * cos(theta), l * sin(theta));
		Vector2 p(px, py);


		return SqrDistancePointToSegment(Vector2_Zero, q, p, NULL) <= r * r;
	}

	bool InFrontOf(const Vector2& a, float angle, const Vector2& dot) {
		Vector2 delta = dot - a;
		if (Math::IsZero(delta)) {
			return true;
		}

		float targetAngle = Math::Deg(atan2(delta[0], delta[1]));
		float diffAngle = targetAngle - angle;

		if (diffAngle >= 270) {
			diffAngle -= 360;
		} else if (diffAngle <= -270) {
			diffAngle += 360;
		}

		if (diffAngle < -90 || diffAngle > 90) {
			return false;
		}

		return true;
	}

	bool InsideCircle(const Vector2& a, float l, const Vector2& center, float r) {
		return IsCircleCircleIntersect(a, l, center, r);
	}

	bool InsideSector(const Vector2& a, float angle, float degree, float l, const Vector2& center, float r) {
		if (!InFrontOf(a, angle, center)) {
			return false;
		}

		Vector2 delta = center - a;
		if (Math::IsZero(delta)) {
			return true;
		}

		if (!InsideCircle(a, l, center, r)) {
			return false;
		}

		float targetAngle = Math::Deg(atan2(delta[0], delta[1]));

		float diffAngle = targetAngle - angle;
		float transAngle = diffAngle + degree / 2;

		while (transAngle > 360) {
			transAngle -= 360;
		}

		while (transAngle < 0) {
			transAngle += 360;
		}

		return transAngle <= degree;
	}

	bool InsideRectangle(const Vector2& a, float angle, float len, float width, const Vector2& center, float r) {
		if (!InFrontOf(a, angle, center)) {
			return false;
		}

		Vector2 delta = center - a;
		if (IsZero(delta)) {
			return true;
		}

		float targetAngle = Math::Deg(atan2(delta[0], delta[1]));
		float diffAngle = targetAngle - angle;

		if (diffAngle >= 270) {
			diffAngle -= 360;
		} else if (diffAngle <= -270) {
			diffAngle += 360;
		}

		if (diffAngle < -90 || diffAngle > 90) {
			return false;
		}

		float diffRadian = Math::Deg(fabs(diffAngle));

		float diffMag = sqrt(Length(delta));

		float changeX = diffMag * cos(diffRadian);
		float changeZ = diffMag * sin(diffRadian);

		if ((changeX < 0 || changeX > len) || (changeZ < 0 || changeZ >(width / 2))) {
			return false;
		}
		return true;
	}

	Vector3 IntersectLineWithPlane(const Vector3& point, const Vector3& direct, const Vector3& planeNormal, const Vector3& planePoint) {
		Vector3 normalize = Normalized(direct);
		float d = Dot(planePoint - point, planeNormal) / Dot(normalize, planeNormal);
		return d * normalize + point;
	}

	static inline float determinant(float v1, float v2, float v3, float v4) {
		return (v1 * v3 - v2 * v4);
	}

	bool SegmentIntersect(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
		const float EPS = 1e-6f;
		float delta = determinant(b[0] - a[0], c[0] - d[0], b[2] - a[2], c[2] - d[2]);
		if (delta <= EPS && delta >= -EPS) {
			return false;
		}

		float namenda = determinant(c[0] - a[0], c[0] - d[0], c[2] - a[2], c[2] - d[2]) / delta;
		if (namenda > 1 || namenda < 0) {
			return false;
		}

		float miu = determinant(b[0] - a[0], c[0] - a[0], b[2] - a[2], c[2] - a[2]) / delta;
		if (miu > 1 || miu < 0) {
			return false;
		}
		return true;
	}

	bool Intersect(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
		if (std::max(a[0], b[0]) >= std::min(c[0], d[0]) &&
			std::max(a[2], b[2]) >= std::min(c[2], d[2]) &&
			std::max(c[0], d[0]) >= std::min(a[0], b[0]) &&
			std::max(c[2], d[2]) >= std::min(a[2], b[2])) {
			Vector3 ac = a - c;
			Vector3 dc = d - c;
			Vector3 bc = b - c;
			Vector3 ca = c - a;
			Vector3 ba = b - a;
			Vector3 da = d - a;

			float crossADC = Cross_Y(ac, dc);
			float crossDBC = Cross_Y(dc, bc);

			if (crossADC * crossDBC >= 0) {
				float crossCBA = Cross_Y(ca, ba);
				float crossDBA = Cross_Y(ba, da);
				if (crossCBA * crossDBA >= 0) {
					return true;
				}
			}
		}
		return false;
	}

	void GetIntersectPoint(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d, Vector3& result) {
		Vector3 base = d - c;
		float d1 = fabsf(Cross_Y(base, a - c));
		float d2 = fabsf(Cross_Y(base, b - c));
		float t = d1 / (d1 + d2);
		result = a + (b - a) * t;
	}
}
