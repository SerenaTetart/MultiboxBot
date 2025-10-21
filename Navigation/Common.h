#pragma once
#include "Vector3.h"


/// <summary>
/// Helper function to insert a vector3 into a float buffer.
/// </summary>
inline void InsertVector3(Vector3* target, int& index, const Vector3* vec, int offset = 0) noexcept
{
    memcpy(target + index, vec + offset, sizeof(Vector3));
    index++;
}


/// <summary>
/// Helper function to scale two vectors and add them. 
/// Used by the smoothing algorithms.
/// </summary>
inline void ScaleAndAddVector3(const Vector3& vec0, float fac0, const Vector3& vec1, float fac1, Vector3& output) noexcept
{
    output.X = vec0.X * fac0 + vec1.X * fac1;
    output.Y = vec0.Y * fac0 + vec1.Y * fac1;
    output.Z = vec0.Z * fac0 + vec1.Z * fac1;
}

static void SmoothPath(const Vector3* input, int inputSize, Vector3* output, int* outputSize, int outputMaxSize) noexcept
{
    InsertVector3(output, *outputSize, input, 0);

    Vector3 result;

    for (int i = 0; i < inputSize - 1; i += 1)
    {
        ScaleAndAddVector3(input[i], 0.75f, input[i + 1], 0.25f, result);
        InsertVector3(output, *outputSize, &result, 0);

        ScaleAndAddVector3(input[i], 0.25f, input[i + 1], 0.75f, result);
        InsertVector3(output, *outputSize, &result, 0);

        if (*outputSize > outputMaxSize - 1) { break; }
    }

    InsertVector3(output, *outputSize, input, inputSize - 1);
}

