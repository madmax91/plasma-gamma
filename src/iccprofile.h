// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

namespace IccProfile
{

struct Chromaticity {
    double rx, ry, gx, gy, bx, by, wx, wy;
};

// sRGB / Rec.709 primaries with a D65 white point.
Chromaticity srgb();

// Reads the colour characteristics block from the connector's EDID in sysfs
// (/sys/class/drm/card*-<connector>/edid). Returns false if not available.
bool readEdidChromaticity(const QString &connector, Chromaticity *out);

// Writes a matrix/shaper profile whose tone curve is base(v)^gamma.
//
// If basePath is non-empty, that profile's primaries and TRC are used as the
// base (LUT-only profiles are rejected). Otherwise a profile is built from
// `chroma` with a pure 2.2 power curve, which is what KWin assumes for SDR
// outputs using the sRGB or EDID colour profile source.
//
// Claiming the display responds as v^(2.2*g) makes KWin pre-compensate with
// v^(1/(2.2*g)), so on screen linear content comes out as L^(1/g): g > 1
// lifts shadows and midtones, g < 1 darkens them. Whites and blacks stay put.
bool write(const QString &path, double gamma, const QString &basePath, const Chromaticity &chroma, QString *error);

}
