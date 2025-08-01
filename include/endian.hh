#ifndef __SYLAR_ENDIAN_H__
#define __SYLAR_ENDIAN_H__

#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN   2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

namespace sylar {

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value)
{
    return (T)bswap_64((uint64_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value)
{
    return (T)bswap_32((uint32_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t)>::type byteswap(T value)
{
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
    #define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#elif BYTE_ORDER == LITTLE_ENDIAN
    #define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
template <class T>
T byteswapOnLittleEndian(T t)
{
    return byteswap(t);
}

template <class T>
T byteswapBigEndian(T t)
{
    return t;
}

#else
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

template <class T>
T byteswapOnLittleEndian(T t)
{
	return t;
}

#endif

}; // namespace sylar
#endif // __SYLAR_ENDIAN_H__
