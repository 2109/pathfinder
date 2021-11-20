#ifndef VECTOR_T_HPP
#define VECTOR_T_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

namespace Math {
	template <typename T, std::size_t dims> class Vector final {
	public:
#if defined(__SSE__) || defined(_M_X64) || _M_IX86_FP >= 1 || defined(__ARM_NEON__)
		alignas(std::is_same<T, float>::value&& dims == 4 ? dims * sizeof(T) : sizeof(T))
#endif
#if (defined(__SSE2__) || defined(_M_X64) || _M_IX86_FP >= 2) || (defined(__ARM_NEON__) && defined(__aarch64__))
			alignas(std::is_same<T, double>::value&& dims == 4 ? dims * sizeof(T) : sizeof(T))
#endif
			T v[dims];

		Vector() noexcept {
			for (std::size_t i = 0; i < dims; ++i)
				v[i] = T(0);
		}

		template <typename ...A>
		explicit Vector(const A... args) noexcept :
			v{ args... } {
		}

		template <std::size_t d, std::size_t c = dims, std::enable_if<(c != d)>* = nullptr>
		Vector(const Math::Vector<T, d>& vec) noexcept {
			for (std::size_t i = 0; i < dims; ++i)
				v[i] = (i < d) ? vec.v[i] : T(0);
		}

		T& operator[](const std::size_t index) noexcept { return v[index]; }
		T operator[](const std::size_t index) const noexcept { return v[index]; }

		T& x() noexcept {
			static_assert(dims >= 1, "");
			return v[0];
		}

		T x() const noexcept {
			static_assert(dims >= 1, "");
			return v[0];
		}

		T& y() noexcept {
			static_assert(dims >= 2, "");
			return v[1];
		}

		T y() const noexcept {
			static_assert(dims >= 2, "");
			return v[1];
		}

		T& z() noexcept {
			static_assert(dims >= 3, "");
			return v[2];
		}

		T z() const noexcept {
			static_assert(dims >= 3, "");
			return v[2];
		}

		T& w() noexcept {
			static_assert(dims >= 4, "");
			return v[3];
		}

		T w() const noexcept {
			static_assert(dims >= 4, "");
			return v[3];
		}
	};

	template <typename T, std::size_t dims>
	bool operator==(const Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			if (lhs.v[i] != rhs.v[i]) return false;
		return true;
	}

	template <typename T, std::size_t dims>
	bool operator!=(const Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			if (lhs.v[i] != rhs.v[i]) return true;
		return false;
	}

	template <typename T, std::size_t dims>
	bool operator<(const Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			if (lhs.v[i] < rhs.v[i]) return true;
		return false;
	}

	template <typename T, std::size_t dims>
	bool operator>(const Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			if (lhs.v[i] > rhs.v[i]) return true;
		return false;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims>& operator+(Vector<T, dims>& vector) noexcept {
		return vector;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> operator-(const Vector<T, dims>& vector) noexcept {
		Vector<T, dims> result;
		for (std::size_t i = 0; i < dims; ++i) result.v[i] = -vector.v[i];
		return result;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> operator+(const Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		Vector<T, dims> result;
		for (std::size_t i = 0; i < dims; ++i)
			result.v[i] = lhs.v[i] + rhs.v[i];
		return result;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims>& operator+=(Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			lhs.v[i] += rhs.v[i];
		return lhs;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> operator-(const Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		Vector<T, dims> result;
		for (std::size_t i = 0; i < dims; ++i)
			result.v[i] = lhs.v[i] - rhs.v[i];
		return result;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims>& operator-=(Vector<T, dims>& lhs,
		const Vector<T, dims>& rhs) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			lhs.v[i] -= rhs.v[i];
		return lhs;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> operator*(const Vector<T, dims>& vector,
		const T scalar) noexcept {
		Vector<T, dims> result;
		for (std::size_t i = 0; i < dims; ++i)
			result.v[i] = vector.v[i] * scalar;
		return result;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims>& operator*=(Vector<T, dims>& vector, const T scalar) noexcept {
		for (auto& c : vector.v) c *= scalar;
		return vector;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> operator/(const Vector<T, dims>& vector,
		const T scalar) noexcept {
		Vector<T, dims> result;
		for (std::size_t i = 0; i < dims; ++i)
			result.v[i] = vector.v[i] / scalar;
		return result;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims>& operator/=(Vector<T, dims>& vector, const T scalar) noexcept {
		for (auto& c : vector.v) c /= scalar;
		return vector;
	}

	template <typename T, std::size_t dims>
	void Zero(Vector<T, dims>& v) {
		for (auto i = 0; i < dims; ++i) {
			v.v[i] = 0.0f;
		}
	}

	template <typename T, std::size_t dims>
	bool IsZero(const Vector<T, dims>& v) {
		for (auto i = 0; i < dims; ++i) {
			if (v.v[i] != (T)0) {
				return false;
			}
		}
		return true;
	}

	template <typename T>
	T Cross(const Vector<T, 2>& lhs, const Vector<T, 2>& rhs) noexcept {
		return lhs[0] * rhs[1] - lhs[1] * rhs[0];
	}

	template <typename T>
	Vector<T, 3> Cross(const Vector<T, 3>& lhs, const Vector<T, 3>& rhs) noexcept {
		return Vector<T, 3>{
			(lhs.v[1] * rhs.v[2]) - (lhs.v[2] * rhs.v[1]),
				(lhs.v[2] * rhs.v[0]) - (lhs.v[0] * rhs.v[2]),
				(lhs.v[0] * rhs.v[1]) - (lhs.v[1] * rhs.v[0])
		};
	}

	template <typename T>
	float Cross_Y(const Vector<T, 3>& lhs, const Vector<T, 3>& rhs) noexcept {
		return lhs[2] * rhs[0] - lhs[0] * rhs[2];
	}

	template <typename T, std::size_t dims>
	std::size_t Length(const Vector<T, dims>& vector) noexcept {
		T length_sqrt{};
		for (const auto& c : vector.v) length_sqrt += c * c;
		return std::sqrt(length_sqrt);
	}

	template <typename T, std::size_t dims>
	T LengthSquared(const Vector<T, dims>& vector) noexcept {
		T length_sqrt{};
		for (const auto& c : vector.v) length_sqrt += c * c;
		return length_sqrt;
	}

	template <typename T, std::size_t dims>
	T Dot(const Vector<T, dims>& lhs, const Vector<T, dims>& rhs) noexcept {
		T result{};
		for (std::size_t i = 0; i < dims; ++i)
			result += lhs.v[i] * rhs[i];
		return result;
	}

	template <typename T, std::size_t dims>
	std::size_t Distance(const Vector<T, dims>& lhs, const Vector<T, dims>& rhs) noexcept {
		T dist_sqrt{};
		for (std::size_t i = 0; i < dims; ++i)
			dist_sqrt += (lhs.v[i] - rhs.v[i]) * (lhs.v[i] - rhs.v[i]);
		return std::sqrt(dist_sqrt);
	}

	template <typename T, std::size_t dims>
	T DistanceSquared(const Vector<T, dims>& lhs, const Vector<T, dims>& rhs) noexcept {
		T dist_sqrt{};
		for (std::size_t i = 0; i < dims; ++i)
			dist_sqrt += (lhs.v[i] - rhs.v[i]) * (lhs.v[i] - rhs.v[i]);
		return dist_sqrt;
	}

	template <typename T, std::size_t dims>
	void Normalize(Vector<T, dims>& vector) noexcept {
		const auto l = Length(vector);
		if (l > T(0))
			for (auto& c : vector.v) c /= l;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> Normalized(const Vector<T, dims>& vector) noexcept {
		Vector<T, dims> result;
		const auto l = Length(vector);
		if (l > T(0))
			for (std::size_t i = 0; i < dims; ++i)
				result.v[i] = vector.v[i] / l;
		return result;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> operator*(const T scalar,
		const Vector<T, dims>& vec) noexcept {
		return vec * scalar;
	}

	template <typename T, std::size_t dims>
	bool IsNormalized(const Vector<T, dims>& vec,
		const T tolerance = std::numeric_limits<T>::epsilon()) noexcept {
		return std::abs(T(1) - lengthSquared(vec)) <= tolerance;
	}

	template <typename T>
	float GetAngle(const Vector<T, 2>& vec) noexcept {
		return std::atan2(vec.v[1], vec.v[0]);
	}

	template <typename T>
	float GetAngle(const Vector<T, 3>& vec,
		const Vector<T, 3>& axis) noexcept {
		constexpr T dx = axis.v[0] - vec.v[0] - vec.v[1] * axis.v[2] + vec.v[2] * axis.v[1];
		constexpr T dy = axis.v[1] - vec.v[1] - vec.v[2] * axis.v[0] + vec.v[0] * axis.v[2];
		constexpr T dz = axis.v[2] - vec.v[2] - vec.v[0] * axis.v[1] + vec.v[1] * axis.v[0];

		return std::atan2(std::sqrt(dx * dx + dy * dy + dz * dz), dot(axis));
	}

	template <typename T>
	float GetAngle(const Vector<T, 4>& vec,
		const Vector<T, 4>& axis) noexcept {
		constexpr T dx = vec.v[3] * axis.v[0] - vec.v[0] * axis.v[3] - vec.v[1] * axis.v[2] + vec.v[2] * axis.v[1];
		constexpr T dy = vec.v[3] * axis.v[1] - vec.v[1] * axis.v[3] - vec.v[2] * axis.v[0] + vec.v[0] * axis.v[2];
		constexpr T dz = vec.v[3] * axis.v[2] - vec.v[2] * axis.v[3] - vec.v[0] * axis.v[1] + vec.v[1] * axis.v[0];

		return std::atan2(std::sqrt(dx * dx + dy * dy + dz * dz), dot(axis));
	}

	template <typename T>
	void Rotate(Vector<T, 2>& vec, const T angle) noexcept {
		const auto sine = std::sin(angle);
		const auto cosine = std::cos(angle);

		const auto tempX = vec.v[0] * cosine - vec.v[1] * sine;
		vec.v[1] = vec.v[1] * cosine + vec.v[0] * sine;
		vec.v[0] = tempX;
	}

	template <typename T>
	void Rotate(Vector<T, 2>& vec, const Math::Vector<T, 2>& point, const T angle) noexcept {
		const auto sine = std::sin(angle);
		const auto cosine = std::cos(angle);

		if (point.v[0] == T(0) || point.v[1] == T(0)) {
			const auto tempX = vec.v[0] * cosine - vec.v[1] * sine;
			vec.v[1] = vec.v[1] * cosine + vec.v[0] * sine;
			vec.v[0] = tempX;
		} else {
			const auto tempX = vec.v[0] - point.v[0];
			const auto tempY = vec.v[1] - point.v[1];

			vec.v[0] = tempX * cosine - tempY * sine + point.v[0];
			vec.v[1] = tempY * cosine + tempX * sine + point.v[1];
		}
	}

	template <typename T>
	void Rotate(Vector<T, 3>& from, const Vector<T, 3>& axis, T angle) {
		Vector<T, 3> o = axis * Dot(axis, from);
		Vector<T, 3> x = from - o;
		Vector<T, 3> y = Cross(axis, from);
		from = (o + x * cos(angle) + y * sin(angle));
	}

	template <typename T, std::size_t dims>
	void Clamp(Vector<T, dims>& vec, const Vector<T, dims>& min, const Vector<T, dims>& max) noexcept {
		for (std::size_t i = 0; i < dims; ++i)
			if (vec.v[i] < min.v[i]) vec.v[i] = min.v[i];
			else if (vec.v[i] > max.v[i]) vec.v[i] = max.v[i];
	}

	template <typename T, std::size_t dims>
	void Smooth(Vector<T, dims>& vec,
		const Vector<T, dims>& target,
		const T elapsedTime,
		const T responseTime) noexcept {
		if (elapsedTime > T(0))
			vec += (target - vec) * (elapsedTime / (elapsedTime + responseTime));
	}

	template <typename T, std::size_t dims>
	void Neg(Vector<T, dims>& vector) noexcept {
		for (auto& c : vector.v) c = -c;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> Max(const Vector<T, dims>& lhs, const Vector<T, dims>& rhs) {
		Vector<T, dims> vec;
		for (std::size_t i = 0; i < dims; ++i)
			vec.v[i] = std::max(lhs.v[0], rhs.v[0]);
		return vec;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> Min(const Vector<T, dims>& lhs, const Vector<T, dims>& rhs) {
		Vector<T, dims> vec;
		for (std::size_t i = 0; i < dims; ++i)
			vec.v[i] = std::min(lhs.v[0], rhs.v[0]);
		return vec;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> Abs(const Vector<T, dims>& vt) {
		Vector<T, dims> vec;
		for (std::size_t i = 0; i < dims; ++i)
			vec.v[i] = std::abs(vt.v[0]);
		return vec;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> Reflect(const Vector<T, dims>& direction, const Vector<T, dims>& normal) {
		return -2.0f * Dot(normal, direction) * normal + direction;
	}

	template <typename T, std::size_t dims>
	Vector<T, dims> Lerp(const Vector<T, dims>& from, const Vector<T, dims>& to, float t) {
		return (from + ((to - from) * Math::Clamp(t, 0, 1)));
	}

	typedef Vector<float, 3> Vector3;

	typedef Vector<float, 2> Vector2;

	static const Vector<float, 3> Vector3_Zero{ 0.0f, 0.0f, 0.0f };

	static const Vector<float, 2> Vector2_Zero{ 0.0f, 0.0f };

	static const float Epsilon = 0.00001F;
}

#include "vector-neron.h"
#include "vector-sse.h"



#endif