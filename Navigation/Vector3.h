#pragma once
#include <array>
#include <string>

class Vector3
{
public:
	float X;
	float Y;
	float Z;
	Vector3();
	Vector3(float x, float y, float z);
	Vector3(const float* floatArray);
	float* ToFloatPointer() const;
	bool Invalid() const;
	Vector3 ToRecast() const;
	Vector3 ToWoW() const;
	std::array<float, 3> ToFloatArray() const;
	float DistanceTo(const Vector3& loc) const;
	float Distance2D(const Vector3& loc) const;
	float Length() const;
	std::string ToJson() const;
	Vector3 operator-(Vector3& other);
	Vector3& operator=(const Vector3& v);
	Vector3 operator*(float scalar);
	Vector3 operator-(const Vector3& other) const;
	// Custom addition operator
	Vector3 operator+(const Vector3& other);
	Vector3 Normalize() const;
	Vector3 Normalize(Vector3 const& v) const;
	float squaredLength() const;

	


	Vector3 operator/(float scalar) const {
		return Vector3(X / scalar, Y / scalar, Z / scalar);
	}

	friend Vector3 operator*(float scalar, const Vector3& vec) {
		return Vector3(vec.X * scalar, vec.Y * scalar, vec.Z * scalar);
	}


	
};