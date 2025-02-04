#ifndef MACROS_H
#define MACROS_H

#include <chrono>

#define PI 3.14159265358979323846
#define SIGN(x) (((x) > 0) - ((x) < 0))
#define DEGREE_TO_RADIAN(x) (x * PI) / 180.f;
#define DISTANCE_2D(x_1,y_1,x_2,y_2) sqrt(pow((x_2 - x_1), 2) + pow((y_2 - y_1), 2));

static const GW::MATH::GVECTORF WORLD_RIGHT{1, 0, 0, 1};

static inline GW::MATH::GVECTORF MultiplyVector2D(GW::MATH::GVECTORF _vec, float _multiplier)
{
	_vec.x *= _multiplier;
	_vec.y *= _multiplier;
	return _vec;
}

static inline GW::MATH::GVECTORF AbsVector(GW::MATH::GVECTORF _vec)
{
	_vec.x = abs(_vec.x);
	_vec.y = abs(_vec.y);
	_vec.z = abs(_vec.z);
	return _vec;
}

static inline GW::MATH::GVECTORF MultiplyVector(GW::MATH::GVECTORF _vec, float _multiplier)
{
	_vec.x *= _multiplier;
	_vec.y *= _multiplier;
	_vec.z *= _multiplier;
	return _vec;
}

static inline GW::MATH::GVECTORF MultiplyVector(GW::MATH::GVECTORF _vec1, GW::MATH::GVECTORF _vec2)
{
	_vec1.x *= _vec2.x;
	_vec1.y *= _vec2.y;
	_vec1.z *= _vec2.z;
	return _vec1;
}

static inline GW::MATH::GVECTORF DivideVector2D(GW::MATH::GVECTORF _vec, float _divisor)
{
	_vec.x /= _divisor;
	_vec.y /= _divisor;
	return _vec;
}

static inline GW::MATH::GVECTORF DivideVector(GW::MATH::GVECTORF _vec, float _divisor)
{
	_vec.x /= _divisor;
	_vec.y /= _divisor;
	_vec.z /= _divisor;
	return _vec;
}

static inline GW::MATH::GVECTORF DivideVector(GW::MATH::GVECTORF _vec1, GW::MATH::GVECTORF _vec2)
{
	_vec1.x /= _vec2.x;
	_vec1.y /= _vec2.y;
	_vec1.z /= _vec2.z;
	return _vec1;
}

static inline bool Equals(const GW::MATH::GVECTORF& _vec1, const GW::MATH::GVECTORF& _vec2)
{
	for (int i = 0; i < 4; i++)
	{
		if (_vec1.data[i] != _vec2.data[i])
			return false;
	}

	return true;
}

static inline bool Equals(const GW::MATH::GMATRIXF& _matrix1, const GW::MATH::GMATRIXF& _matrix2)
{
	for (int i = 0; i < 16; i++)
	{
		if (_matrix1.data[i] != _matrix2.data[i])
			return false;
	}

	return true;
}

static inline GW::MATH::GVECTORF GetGAABBMMFExtent(const GW::MATH::GAABBMMF& gaabbmmf)
{
	return {
		(gaabbmmf.max.x - gaabbmmf.min.x) * 0.5f,
		(gaabbmmf.max.y - gaabbmmf.min.y) * 0.5f,
		(gaabbmmf.max.y - gaabbmmf.min.z) * 0.5f,
	};
}

static inline GW::MATH::GVECTORF GetGAABBMMFCenter(const GW::MATH::GAABBMMF& gaabbmmf)
{
	GW::MATH::GVECTORF center;
	GW::MATH::GVector::LerpF(gaabbmmf.min, gaabbmmf.max, 0.5f, center);
	return center;
}

static float Lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

static bool IsInRange(float _var, float _minInclusive, float _maxExclusive)
{
	return _var >= _minInclusive && _var < _maxExclusive;
}

static long long GetNow()
{
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// TODO convert to macro
static inline float Distance2D(const GW::MATH::GVECTORF& _vec1, const GW::MATH::GVECTORF& _vec2)
{
	return sqrt(pow(_vec1.x - _vec2.x, 2) + pow(_vec1.y - _vec2.y, 2));
}

static GW::MATH::GVECTORF StringToGVector(const std::string& str)
{
	GW::MATH::GVECTORF output{};
	std::stringstream stream(str);
	std::string temp;

	int i = 0;
	while (std::getline(stream, temp, ','))
	{
		output.data[i] = std::stof(temp);
		i++;
	}

	if (i < 4)
		output.w = 1;

	return output;
}

// Trims leading or trailing digits
static std::string TrimDigits(const std::string& str) 
{
	if (str.size() == 0)
		return "";

	int substrStart = 0;

	for (int i = 0; i < str.size(); i++)
	{
		if (std::isdigit(str[i]))
			substrStart++;
		else
			break;
	}

	int substrCount = str.size() - substrStart;

	for (int i = str.size() - 1; i >= 0; i--)
	{
		if (std::isdigit(str[i]))
			substrCount--;
		else
			break;
	}

	return str.substr(substrStart, substrCount);
}
#endif