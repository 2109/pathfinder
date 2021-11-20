#ifndef VECTOR_NERON_H
#define VECTOR_NERON_H

#include "vector-t.h"

#ifdef __ARM_NEON__
#  include <arm_neon.h>

namespace Math {
	template <>
	inline Vector<float, 4> operator-(const Vector<float, 4>& vector) noexcept {
		Vector<float, 4> result;
		vst1q_f32(result.v, vnegq_f32(vld1q_f32(vector.v)));
		return result;
	}

	template <>
	inline void neg(Vector<float, 4>& vector) noexcept {
		vst1q_f32(vector.v, vnegq_f32(vld1q_f32(vector.v)));
	}

	template <>
	 inline Vector<float, 4> operator+(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		Vector<float, 4> result;
		vst1q_f32(result.v, vaddq_f32(vld1q_f32(lhs.v), vld1q_f32(rhs.v)));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator+=(Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		vst1q_f32(lhs.v, vaddq_f32(vld1q_f32(lhs.v), vld1q_f32(rhs.v)));
		return lhs;
	}

	template <>
	 inline Vector<float, 4> operator-(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		Vector<float, 4> result;
		vst1q_f32(result.v, vsubq_f32(vld1q_f32(lhs.v), vld1q_f32(rhs.v)));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator-=(Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		vst1q_f32(lhs.v, vsubq_f32(vld1q_f32(lhs.v), vld1q_f32(rhs.v)));
		return lhs;
	}

	template <>
	 inline Vector<float, 4> operator*(const Vector<float, 4>& vector,
		const float scalar) noexcept {
		Vector<float, 4> result;
		const auto s = vdupq_n_f32(scalar);
		vst1q_f32(result.v, vmulq_f32(vld1q_f32(vector.v), s));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator*=(Vector<float, 4>& vector,
		const float scalar) noexcept {
		const auto s = vdupq_n_f32(scalar);
		vst1q_f32(vector.v, vmulq_f32(vld1q_f32(vector.v), s));
		return vector;
	}

	template <>
	 inline Vector<float, 4> operator/(const Vector<float, 4>& vector,
		const float scalar) noexcept {
		Vector<float, 4> result;
		const auto s = vdupq_n_f32(scalar);
		vst1q_f32(result.v, vdivq_f32(vld1q_f32(vector.v), s));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator/=(Vector<float, 4>& vector,
		const float scalar) noexcept {
		const auto s = vdupq_n_f32(scalar);
		vst1q_f32(vector.v, vdivq_f32(vld1q_f32(vector.v), s));
		return vector;
	}

	template <>
	 inline float dot(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		const auto t1 = vmulq_f32(vld1q_f32(lhs.v), vld1q_f32(rhs.v));
		const auto t2 = vaddq_f32(t1, vrev64q_f32(t1));
		const auto t3 = vaddq_f32(t2, vcombine_f32(vget_high_f32(t2), vget_low_f32(t2)));
		const float result = vgetq_lane_f32(t3, 0);
		return result;
	}
}

#endif // __ARM_NEON__

#endif