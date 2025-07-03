#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <cstdlib>
//#include <stdexcept>

const Rgba8 Rgba8::WHITE = Rgba8(255, 255, 255, 255);
const Rgba8 Rgba8::BLACK = Rgba8(0, 0, 0, 255);
const Rgba8 Rgba8::RED = Rgba8(255, 0, 0, 255);
const Rgba8 Rgba8::GREEN = Rgba8(0, 255, 0, 255);
const Rgba8 Rgba8::BLUE = Rgba8(0, 0, 255, 255);
const Rgba8 Rgba8::YELLOW = Rgba8(255, 255, 0, 255);
const Rgba8 Rgba8::GREY = Rgba8(120, 120, 120, 255);
const Rgba8 Rgba8::DARKGREY = Rgba8(45, 45, 45, 255);
const Rgba8 Rgba8::TEAL = Rgba8(0, 190, 190, 255);
const Rgba8 Rgba8::CYAN = Rgba8(0, 255, 255, 255);
const Rgba8 Rgba8::MAGENTA = Rgba8(255, 0, 255, 255);
const Rgba8 Rgba8::AQUA = Rgba8(127, 185, 212, 255);
const Rgba8 Rgba8::MINTGREEN = Rgba8(152, 255, 152, 255);
const Rgba8 Rgba8::PEACH = Rgba8(255, 218, 185, 255);
const Rgba8 Rgba8::LAVENDER = Rgba8(200, 200, 250, 255);
const Rgba8 Rgba8::MISTBLUE = Rgba8(130, 150, 255, 255);

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha /*= 255*/)
    : r(red)
    , g(green)
    , b(blue)
    , a(alpha)
{
}

void Rgba8::SetFromText(char const* text)
{
	if (!text) 
	{
		return;
	}

	Strings parts = SplitStringOnDelimiter(text, ',');
	size_t size = parts.size();

	if (size < 3 || size > 4) 
	{
		return;
	}

	// Trim whitespace and convert to integers
	r = static_cast<unsigned char>(atoi(parts[0].c_str()));
	g = static_cast<unsigned char>(atoi(parts[1].c_str()));
	b = static_cast<unsigned char>(atoi(parts[2].c_str()));
	a = (size == 4) ? static_cast<unsigned char>(atoi(parts[3].c_str())) : 255; 
}

void Rgba8::GetAsFloats(float* colorAsFloats) const
{
	if (!colorAsFloats)
	{
		return;
	}

	colorAsFloats[0] = static_cast<float>(r) / 255.0f;
	colorAsFloats[1] = static_cast<float>(g) / 255.0f;
	colorAsFloats[2] = static_cast<float>(b) / 255.0f;
	colorAsFloats[3] = static_cast<float>(a) / 255.0f;
}

Rgba8 InterpolateRgba8(Rgba8 start, Rgba8 end, float fractionOfEnd)
{
	float rForInter = Interpolate(NormalizeByte(start.r), NormalizeByte(end.r), fractionOfEnd);
	float gForInter = Interpolate(NormalizeByte(start.g), NormalizeByte(end.g), fractionOfEnd);
	float bForInter = Interpolate(NormalizeByte(start.b), NormalizeByte(end.b), fractionOfEnd);
	float aForInter = Interpolate(NormalizeByte(start.a), NormalizeByte(end.a), fractionOfEnd);

	return Rgba8(DenormalizeByte(rForInter), DenormalizeByte(gForInter), DenormalizeByte(bForInter), DenormalizeByte(aForInter));
}

