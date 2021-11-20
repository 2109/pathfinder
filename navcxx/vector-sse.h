#ifndef VECTOR_SSE_H
#define VECTOR_SSE_H

#include "vector.h"

#if defined(__SSE__) || defined(_M_X64) || _M_IX86_FP >= 1
#  include <xmmintrin.h>
#endif

#if defined(__SSE2__) || defined(_M_X64) || _M_IX86_FP >= 2
#  include <emmintrin.h>
#endif

#ifdef __SSE3__
#  include <pmmintrin.h>
#endif // __SSE3__

namespace Math {
#if defined(__SSE__) || defined(_M_X64) || _M_IX86_FP >= 1
	template <>
	inline Vector<float, 4> operator-(const Vector<float, 4>& vector) noexcept {
		Vector<float, 4> result;
		const auto z = _mm_setzero_ps();
		_mm_store_ps(result.v, _mm_sub_ps(z, _mm_load_ps(vector.v)));
		return result;
	}

	template <>
	inline void Neg(Vector<float, 4>& vector) noexcept {
		const auto z = _mm_setzero_ps();
		_mm_store_ps(vector.v, _mm_sub_ps(z, _mm_load_ps(vector.v)));
	}

	template <>
	 inline Vector<float, 4> operator+(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		Vector<float, 4> result;
		_mm_store_ps(result.v, _mm_add_ps(_mm_load_ps(lhs.v), _mm_load_ps(rhs.v)));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator+=(Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		_mm_store_ps(lhs.v, _mm_add_ps(_mm_load_ps(lhs.v), _mm_load_ps(rhs.v)));
		return lhs;
	}

	template <>
	 inline Vector<float, 4> operator-(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		Vector<float, 4> result;
		_mm_store_ps(result.v, _mm_sub_ps(_mm_load_ps(lhs.v), _mm_load_ps(rhs.v)));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator-=(Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		_mm_store_ps(lhs.v, _mm_sub_ps(_mm_load_ps(lhs.v), _mm_load_ps(rhs.v)));
		return lhs;
	}

	template <>
	 inline Vector<float, 4> operator*(const Vector<float, 4>& vector,
		const float scalar) noexcept {
		Vector<float, 4> result;
		const auto s = _mm_set1_ps(scalar);
		_mm_store_ps(result.v, _mm_mul_ps(_mm_load_ps(vector.v), s));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator*=(Vector<float, 4>& vector,
		const float scalar) noexcept {
		const auto s = _mm_set1_ps(scalar);
		_mm_store_ps(vector.v, _mm_mul_ps(_mm_load_ps(vector.v), s));
		return vector;
	}

	template <>
	 inline Vector<float, 4> operator/(const Vector<float, 4>& vector,
		const float scalar) noexcept {
		Vector<float, 4> result;
		const auto s = _mm_set1_ps(scalar);
		_mm_store_ps(result.v, _mm_div_ps(_mm_load_ps(vector.v), s));
		return result;
	}

	template <>
	inline Vector<float, 4>& operator/=(Vector<float, 4>& vector,
		const float scalar) noexcept {
		const auto s = _mm_set1_ps(scalar);
		_mm_store_ps(vector.v, _mm_div_ps(_mm_load_ps(vector.v), s));
		return vector;
	}

#  ifndef __SSE3__
	template <>
	 inline std::size_t Length(const Vector<float, 4>& vector) noexcept {
		const auto v = _mm_load_ps(vector.v);
		const auto mul = _mm_mul_ps(v, v);
		const auto shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3));
		const auto sum = _mm_add_ps(mul, shuf);
		const auto mov = _mm_movehl_ps(shuf, sum);
		const auto sqrt = _mm_sqrt_ps(_mm_add_ss(sum, mov));
		return _mm_cvtss_f32(sqrt);
	}

	template <>
	 inline float LengthSquared(const Vector<float, 4>& vector) noexcept {
		const auto v = _mm_load_ps(vector.v);
		const auto mul = _mm_mul_ps(v, v);
		const auto shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3));
		const auto sum = _mm_add_ps(mul, shuf);
		const auto mov = _mm_movehl_ps(shuf, sum);
		return _mm_cvtss_f32(_mm_add_ss(sum, mov));
	}

	template <>
	 inline float Dot(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		const auto mul = _mm_mul_ps(_mm_load_ps(lhs.v), _mm_load_ps(rhs.v));
		const auto shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 1, 0, 3));
		const auto sum = _mm_add_ps(mul, shuf);
		const auto mov = _mm_movehl_ps(shuf, sum);
		return _mm_cvtss_f32(_mm_add_ss(sum, mov));
	}
#  endif // __SSE3__
#endif // SSE

#if defined(__SSE2__) || defined(_M_X64) || _M_IX86_FP >= 2
#  ifndef __AVX__
	template <>
	inline Vector<double, 4> operator-(const Vector<double, 4>& vector) noexcept {
		Vector<double, 4> result;
		const auto z = _mm_setzero_pd();
		_mm_store_pd(&result.v[0], _mm_sub_pd(z, _mm_load_pd(&vector.v[0])));
		_mm_store_pd(&result.v[2], _mm_sub_pd(z, _mm_load_pd(&vector.v[2])));
		return result;
	}

	template <>
	inline void Neg(Vector<double, 4>& vector) noexcept {
		const auto z = _mm_setzero_pd();
		_mm_store_pd(&vector.v[0], _mm_sub_pd(z, _mm_load_pd(&vector.v[0])));
		_mm_store_pd(&vector.v[2], _mm_sub_pd(z, _mm_load_pd(&vector.v[2])));
	}

	template <>
	 inline Vector<double, 4> operator+(const Vector<double, 4>& lhs,
		const Vector<double, 4>& rhs) noexcept {
		Vector<double, 4> result;
		_mm_store_pd(&result.v[0], _mm_add_pd(_mm_load_pd(&lhs.v[0]), _mm_load_pd(&rhs.v[0])));
		_mm_store_pd(&result.v[2], _mm_add_pd(_mm_load_pd(&lhs.v[2]), _mm_load_pd(&rhs.v[2])));
		return result;
	}

	template <>
	inline Vector<double, 4>& operator+=(Vector<double, 4>& lhs,
		const Vector<double, 4>& rhs) noexcept {
		_mm_store_pd(&lhs.v[0], _mm_add_pd(_mm_load_pd(&lhs.v[0]), _mm_load_pd(&rhs.v[0])));
		_mm_store_pd(&lhs.v[2], _mm_add_pd(_mm_load_pd(&lhs.v[2]), _mm_load_pd(&rhs.v[2])));
		return lhs;
	}

	template <>
	 inline Vector<double, 4> operator-(const Vector<double, 4>& lhs,
		const Vector<double, 4>& rhs) noexcept {
		Vector<double, 4> result;
		_mm_store_pd(&result.v[0], _mm_sub_pd(_mm_load_pd(&lhs.v[0]), _mm_load_pd(&rhs.v[0])));
		_mm_store_pd(&result.v[2], _mm_sub_pd(_mm_load_pd(&lhs.v[2]), _mm_load_pd(&rhs.v[2])));
		return result;
	}

	template <>
	inline Vector<double, 4>& operator-=(Vector<double, 4>& lhs,
		const Vector<double, 4>& rhs) noexcept {
		_mm_store_pd(&lhs.v[0], _mm_sub_pd(_mm_load_pd(&lhs.v[0]), _mm_load_pd(&rhs.v[0])));
		_mm_store_pd(&lhs.v[2], _mm_sub_pd(_mm_load_pd(&lhs.v[2]), _mm_load_pd(&rhs.v[2])));
		return lhs;
	}

	template <>
	 inline Vector<double, 4> operator*(const Vector<double, 4>& vector,
		const double scalar) noexcept {
		Vector<double, 4> result;
		const auto s = _mm_set1_pd(scalar);
		_mm_store_pd(&result.v[0], _mm_mul_pd(_mm_load_pd(&vector.v[0]), s));
		_mm_store_pd(&result.v[2], _mm_mul_pd(_mm_load_pd(&vector.v[2]), s));
		return result;
	}

	template <>
	inline Vector<double, 4>& operator*=(Vector<double, 4>& vector,
		const double scalar) noexcept {
		const auto s = _mm_set1_pd(scalar);
		_mm_store_pd(&vector.v[0], _mm_mul_pd(_mm_load_pd(&vector.v[0]), s));
		_mm_store_pd(&vector.v[2], _mm_mul_pd(_mm_load_pd(&vector.v[2]), s));
		return vector;
	}

	template <>
	 inline Vector<double, 4> operator/(const Vector<double, 4>& vector,
		const double scalar) noexcept {
		Vector<double, 4> result;
		const auto s = _mm_set1_pd(scalar);
		_mm_store_pd(&result.v[0], _mm_div_pd(_mm_load_pd(&vector.v[0]), s));
		_mm_store_pd(&result.v[2], _mm_div_pd(_mm_load_pd(&vector.v[2]), s));
		return result;
	}

	template <>
	inline Vector<double, 4>& operator/=(Vector<double, 4>& vector,
		const double scalar) noexcept {
		const auto s = _mm_set1_pd(scalar);
		_mm_store_pd(&vector.v[0], _mm_div_pd(_mm_load_pd(&vector.v[0]), s));
		_mm_store_pd(&vector.v[2], _mm_div_pd(_mm_load_pd(&vector.v[2]), s));
		return vector;
	}
#  endif // __AVX__
#endif // SSE2

#ifdef __SSE3__
	template <>
	 inline float length(const Vector<float, 4>& vector) noexcept {
		const auto v = _mm_load_ps(vector.v);
		const auto t1 = _mm_mul_ps(v, v);
		const auto t2 = _mm_hadd_ps(t1, t1);
		const auto t3 = _mm_hadd_ps(t2, t2);
		const auto s = _mm_sqrt_ps(t3);
		return _mm_cvtss_f32(s);
	}

	template <>
	 inline float lengthSquared(const Vector<float, 4>& vector) noexcept {
		const auto v = _mm_load_ps(vector.v);
		const auto t1 = _mm_mul_ps(v, v);
		const auto t2 = _mm_hadd_ps(t1, t1);
		const auto t3 = _mm_hadd_ps(t2, t2);
		return _mm_cvtss_f32(t3);
	}

	template <>
	 inline float dot(const Vector<float, 4>& lhs,
		const Vector<float, 4>& rhs) noexcept {
		const auto t1 = _mm_mul_ps(_mm_load_ps(lhs.v), _mm_load_ps(rhs.v));
		const auto t2 = _mm_hadd_ps(t1, t1);
		const auto t3 = _mm_hadd_ps(t2, t2);
		return _mm_cvtss_f32(t3);
	}
#endif // __SSE3__
}

#endif