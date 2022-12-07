#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>

template<typename T>
using ptr = std::shared_ptr<T>;

template<typename T>
using opt = std::optional<T>;

#define match(expr,t,some,none) if(auto t = expr;t.has_value()) {some;} else {none;}
#define vkmc_count(arr) sizeof(arr) / sizeof(arr[0])

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;


#define vkmc_assert(expr) if(!(expr)) {__debugbreak();exit(-1);}

#ifdef DEBUG
#define vkmc_debug_assert(expr) vkmc_assert(expr)
#else
#define vkmc_debug_assert(expr) void;
#endif


/// rule of face encoding on cube
/// <summary>
///     front face        back face
///     --------          -------
///	   /  1   / |        /  4   / |
///	  /      /  |       /      /  |
///	  ------- 2 |       ------- 5 |
///  |       |  /      |       |  |
///	 |   0   | /       |   3   |  /
///  |       |/        |       | /
///   -------           ------- 
///   numbering of faces
/// </summary>
enum FaceCode {
	FACE_CODE_POSITIVE_X = 0,
	FACE_CODE_NEGATIVE_X = 3,
	FACE_CODE_POSITIVE_Y = 1,
	FACE_CODE_NEGATIVE_Y = 4,
	FACE_CODE_POSITIVE_Z = 2,
	FACE_CODE_NEGATIVE_Z = 5
};

inline FaceCode OppsiteFaceCode(FaceCode code)
{
	return (FaceCode)((code + 3) % 6);
}