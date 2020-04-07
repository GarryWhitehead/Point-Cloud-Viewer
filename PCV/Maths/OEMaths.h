
#pragma once

#include "Mat2.h"
#include "Mat3.h"
#include "Mat4.h"
#include "MatN.h"
#include "Quat.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "VecN.h"

#include <assert.h>
#include <cstdint>

#define M_DBL_PI 6.28318530718

#define M_HALF_PI 1.57079632679

#define M_EPSILON 0.00001

// this should be defined if using the maths library with Vulkan to compensate for the difference in the y coord
#define USE_VULKAN_COORDS 1

namespace OEMaths
{

// some popular maths conversions (haven't decided were to locate these yet!)
template <typename T>
T radians(const T deg)
{
	return deg * static_cast<T>(M_PI / 180.0);
}

}    // namespace OEMaths
