#pragma once
#include <cmath>

struct Vector2 {
	float x, z;

	inline Vector2 operator-(const Vector2& other) const {
		return Vector2(x - other.x, z - other.z);
	}

	inline Vector2 operator+(const Vector2& other) const {
		return Vector2(x + other.x, z + other.z);
	}

	inline Vector2 operator*(float scalar) const {
		return Vector2(x * scalar, z * scalar);
	}

	inline float magnitude() const {
		return std::sqrt(x * x + z * z);
	}

	inline float distance(const Vector2& other) const {
		float dx = other.x - x;
		float dz = other.z - z;
		return std::sqrt(dx * dx + dz * dz);
	}

	inline Vector2 normalized() const {
		float mag = magnitude();
		if (mag > 0) {
			return Vector2(x / mag, z / mag);
		}
		return Vector2(0, 0);
	}

	inline float dot(const Vector2& other) const {
		return x * other.x + z * other.z;
	}

	inline Vector2 perpendicular() const {
		return Vector2(-z, x);
	}
};

inline float clamp(float num, float a, float b) {
	return min(max(num, a), b);
}

struct Vector3 {
	float x, y, z;

	inline Vector3 operator-(const Vector3& other) const {
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	inline float magnitude() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	inline float magnitudeXZ() const {
		return std::sqrt(x * x + z * z);
	}

	inline float dot(const Vector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}
	inline Vector2 xz() const {
		return Vector2(x, z);
	}
	inline float distance(const Vector3& other) const {
		float dx = this->x - other.x;
		float dy = this->y - other.y;
		float dz = this->z - other.z;
		return std::sqrt(dx*dx + dy*dy + dz*dz);
	}
	inline float distanceXZ(const Vector3& other) const {
		return xz().distance(other.xz());
	}

	inline float getCosAngle(const Vector3& other) const {
		if (magnitude() == 0.0f || other.magnitude() == 0.0f) {
			return 1.0f;
		}
		else {
			return clamp(dot(other) / (magnitude() * other.magnitude()), -1.0f, 1.0f);
		}
	}
};