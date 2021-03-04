////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2018 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#ifndef SFML_VECTOR4_HPP
#define SFML_VECTOR4_HPP


namespace sf
{
template <typename T>
class Vector4
{
public:
    Vector4();

    Vector4(T X, T Y, T Z, T W);

    template <typename U>
    explicit Vector4(const Vector4<U>& vector);

    T x;
    T y;
    T z;
    T w;
};


template <typename T>
Vector4<T> operator -(const Vector4<T>& right);

template <typename T>
Vector4<T>& operator +=(Vector4<T>& left, const Vector4<T>& right);

template <typename T>
Vector4<T>& operator -=(Vector4<T>& left, const Vector4<T>& right);

template <typename T>
Vector4<T> operator +(const Vector4<T>& left, const Vector4<T>& right);

template <typename T>
Vector4<T> operator -(const Vector4<T>& left, const Vector4<T>& right);

template <typename T>
Vector4<T> operator *(const Vector4<T>& left, T right);

template <typename T>
Vector4<T> operator *(T left, const Vector4<T>& right);

template <typename T>
Vector4<T>& operator *=(Vector4<T>& left, T right);

template <typename T>
Vector4<T> operator /(const Vector4<T>& left, T right);

template <typename T>
Vector4<T>& operator /=(Vector4<T>& left, T right);

template <typename T>
bool operator ==(const Vector4<T>& left, const Vector4<T>& right);

template <typename T>
bool operator !=(const Vector4<T>& left, const Vector4<T>& right);

#include <SFML/System/Vector4.inl>

// Define the most common types
typedef Vector4<int>          Vector4i;
typedef Vector4<unsigned int> Vector4u;
typedef Vector4<float>        Vector4f;

} // namespace sf


#endif // SFML_VECTOR4_HPP
