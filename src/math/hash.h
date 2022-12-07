#include "xxhash.h"

constexpr uint64_t __vkmc_hash_seed = 1145141919;

inline uint64_t hash64(void* buffer,size_t size)
{
	return XXH64(buffer, size, __vkmc_hash_seed);
}

template<typename T>
inline uint64_t hash64(const T& obj)
{
	return XXH64(&obj, sizeof(obj));
}

inline uint32_t hash32(void* buffer,size_t size)
{
	return XXH32(buffer, size, __vkmc_hash_seed);
}

template<typename T>
inline uint32_t hash32(const T& obj)
{
	return XXH32(&obj, size, __vkmc_hash_seed);
}