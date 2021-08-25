

float3 ApplySRGBCurve(float3 x)
{
    // Approximately pow(x, 1.0 / 2.2)
    return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

float3 RemoveSRGBCurve(float3 x)
{
    // Approximately pow(x, 2.2)
    return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

// Encodes a smooth logarithmic gradient for even distribution of precision natural to vision
float LinearToLogLuminance(float x, float gamma = 4.0)
{
    return log2(lerp(1, exp2(gamma), x)) / gamma;
}

// This assumes the default color gamut found in sRGB and REC709.  The color primaries determine these
// coefficients.  Note that this operates on linear values, not gamma space.
float RGBToLuminance(float3 x)
{
    return dot(x, float3(0.212671, 0.715160, 0.072169));        // Defined by sRGB/Rec.709 gamut
}

float Max3(float3 x)
{
    return max(x.x, max(x.y, x.z));
}

// This is the same as above, but converts the linear luminance value to a more subjective "perceived luminance",
// which could be called the Log-Luminance.
float RGBToLogLuminance(float3 x, float gamma = 4.0)
{
    return LinearToLogLuminance(RGBToLuminance(x), gamma);
}

// A fast invertible tone map that preserves color (Reinhard)
float3 TM(float3 rgb)
{
    return rgb / (1 + RGBToLuminance(rgb));
}

// Inverse of preceding function
float3 ITM(float3 rgb)
{
    return rgb / (1 - RGBToLuminance(rgb));
}

// 8-bit should range from 16 to 235
float3 RGBFullToLimited8bit(float3 x)
{
    return saturate(x) * 219.0 / 255.0 + 16.0 / 255.0;
}

float3 RGBLimitedToFull8bit(float3 x)
{
    return saturate((x - 16.0 / 255.0) * 255.0 / 219.0);
}

// 10-bit should range from 64 to 940
float3 RGBFullToLimited10bit(float3 x)
{
    return saturate(x) * 876.0 / 1023.0 + 64.0 / 1023.0;
}

float3 RGBLimitedToFull10bit(float3 x)
{
    return saturate((x - 64.0 / 1023.0) * 1023.0 / 876.0);
}


// The OETF recommended for content shown on HDTVs.  This "gamma ramp" may increase contrast as
// appropriate for viewing in a dark environment.  Always use this curve with Limited RGB as it is
// used in conjunction with HDTVs.
float3 ApplyREC709Curve(float3 x)
{
    return x < 0.0181 ? 4.5 * x : 1.0993 * pow(x, 0.45) - 0.0993;
}

float3 RemoveREC709Curve(float3 x)
{
    return x < 0.08145 ? x / 4.5 : pow((x + 0.0993) / 1.0993, 1.0 / 0.45);
}

// This is the new HDR transfer function, also called "PQ" for perceptual quantizer.  Note that REC2084
// does not also refer to a color space.  REC2084 is typically used with the REC2020 color space.
float3 ApplyREC2084Curve(float3 L)
{
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;
    float3 Lp = pow(L, m1);
    return pow((c1 + c2 * Lp) / (1 + c3 * Lp), m2);
}

float3 RemoveREC2084Curve(float3 N)
{
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;
    float3 Np = pow(N, 1 / m2);
    return pow(max(Np - c1, 0) / (c2 - c3 * Np), 1 / m1);
}

//
// Color space conversions
//
// These assume linear (not gamma-encoded) values.  A color space conversion is a change
// of basis (like in Linear Algebra).  Since a color space is defined by three vectors--
// the basis vectors--changing space involves a matrix-vector multiplication.  Note that
// changing the color space may result in colors that are "out of bounds" because some
// color spaces have larger gamuts than others.  When converting some colors from a wide
// gamut to small gamut, negative values may result, which are inexpressible in that new
// color space.
//
// It would be ideal to build a color pipeline which never throws away inexpressible (but
// perceivable) colors.  This means using a color space that is as wide as possible.  The
// XYZ color space is the neutral, all-encompassing color space, but it has the unfortunate
// property of having negative values (specifically in X and Z).  To correct this, a further
// transformation can be made to X and Z to make them always positive.  They can have their
// precision needs reduced by dividing by Y, allowing X and Z to be packed into two UNORM8s.
// This color space is called YUV for lack of a better name.
//

// Note:  Rec.709 and sRGB share the same color primaries and white point.  Their only difference
// is the transfer curve used.

float3 REC709toREC2020(float3 RGB709)
{
    static const float3x3 ConvMat =
    {
        0.627402, 0.329292, 0.043306,
        0.069095, 0.919544, 0.011360,
        0.016394, 0.088028, 0.895578
    };
    return mul(ConvMat, RGB709);
}

float3 REC2020toREC709(float3 RGB2020)
{
    static const float3x3 ConvMat =
    {
        1.660496, -0.587656, -0.072840,
        -0.124547, 1.132895, -0.008348,
        -0.018154, -0.100597, 1.118751
    };
    return mul(ConvMat, RGB2020);
}

float3 REC709toDCIP3(float3 RGB709)
{
    static const float3x3 ConvMat =
    {
        0.822458, 0.177542, 0.000000,
        0.033193, 0.966807, 0.000000,
        0.017085, 0.072410, 0.910505
    };
    return mul(ConvMat, RGB709);
}

float3 DCIP3toREC709(float3 RGB709)
{
    static const float3x3 ConvMat =
    {
        1.224947, -0.224947, 0.000000,
        -0.042056, 1.042056, 0.000000,
        -0.019641, -0.078651, 1.098291
    };
    return mul(ConvMat, RGB709);
}


#define COLOR_FORMAT_LINEAR           0
#define COLOR_FORMAT_sRGB_FULL        1
#define COLOR_FORMAT_sRGB_LIMITED     2
#define COLOR_FORMAT_HDR10            3

float3 ApplyDisplayProfile(float3 x, int DisplayFormat)
{
    switch (DisplayFormat)
    {
    default:
    case COLOR_FORMAT_LINEAR:
        return x;
    case COLOR_FORMAT_sRGB_FULL:
        return ApplySRGBCurve(x);
    case COLOR_FORMAT_sRGB_LIMITED:
        return RGBFullToLimited10bit(ApplySRGBCurve(x));
    case COLOR_FORMAT_HDR10:
        return ApplyREC2084Curve(REC709toREC2020(x));
    };
}

float3 RemoveDisplayProfile(float3 x, int DisplayFormat)
{
    switch (DisplayFormat)
    {
    default:
    case COLOR_FORMAT_LINEAR:
        return x;
    case COLOR_FORMAT_sRGB_FULL:
        return RemoveSRGBCurve(x);
    case COLOR_FORMAT_sRGB_LIMITED:
        return RemoveSRGBCurve(RGBLimitedToFull10bit(x));
    case COLOR_FORMAT_HDR10:
        return REC2020toREC709(RemoveREC2084Curve(x));
    };
}

float3 ConvertColor(float3 x, int FromFormat, int ToFormat)
{
    if (FromFormat == ToFormat)
        return x;

    return ApplyDisplayProfile(RemoveDisplayProfile(x, FromFormat), ToFormat);
}