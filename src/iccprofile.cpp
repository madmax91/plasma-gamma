// SPDX-License-Identifier: MIT
#include "iccprofile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <lcms2.h>
#include <vector>

namespace IccProfile
{

Chromaticity srgb()
{
    return {0.64, 0.33, 0.30, 0.60, 0.15, 0.06, 0.3127, 0.3290};
}

bool readEdidChromaticity(const QString &connector, Chromaticity *out)
{
    const QDir drm(QStringLiteral("/sys/class/drm"));
    const QStringList entries = drm.entryList({QStringLiteral("card*-") + connector}, QDir::Dirs | QDir::System);
    for (const QString &entry : entries) {
        QFile f(drm.filePath(entry) + QStringLiteral("/edid"));
        if (!f.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QByteArray edid = f.read(128);
        static const char header[] = "\x00\xff\xff\xff\xff\xff\xff\x00";
        if (edid.size() < 35 || !edid.startsWith(QByteArray(header, 8))) {
            continue;
        }
        const auto b = [&edid](int i) {
            return static_cast<unsigned char>(edid.at(i));
        };
        // EDID 1.4 bytes 25-34: 10-bit CIE xy values, low bits packed in 25/26
        const auto v = [&](int hi, int loByte, int shift) {
            return ((b(hi) << 2) | ((b(loByte) >> shift) & 0x3)) / 1024.0;
        };
        Chromaticity c{v(27, 25, 6), v(28, 25, 4), v(29, 25, 2), v(30, 25, 0),
                       v(31, 26, 6), v(32, 26, 4), v(33, 26, 2), v(34, 26, 0)};
        if (c.ry <= 0 || c.gy <= 0 || c.by <= 0 || c.wy <= 0) {
            return false;
        }
        *out = c;
        return true;
    }
    return false;
}

static cmsToneCurve *raisedCurve(const cmsToneCurve *base, double gamma)
{
    constexpr int samples = 4096;
    std::vector<cmsFloat32Number> values(samples);
    for (int i = 0; i < samples; ++i) {
        const cmsFloat32Number x = static_cast<cmsFloat32Number>(i) / (samples - 1);
        values[i] = std::pow(std::clamp(cmsEvalToneCurveFloat(base, x), 0.0f, 1.0f), static_cast<float>(gamma));
    }
    return cmsBuildTabulatedToneCurveFloat(nullptr, samples, values.data());
}

bool write(const QString &path, double gamma, const QString &basePath, const Chromaticity &c, QString *error)
{
    cmsHPROFILE profile = nullptr;
    if (!basePath.isEmpty()) {
        profile = cmsOpenProfileFromFile(QFile::encodeName(basePath).constData(), "r");
        if (!profile) {
            *error = QStringLiteral("cannot read ICC profile %1").arg(basePath);
            return false;
        }
    } else {
        const cmsCIExyY white{c.wx, c.wy, 1.0};
        const cmsCIExyYTRIPLE primaries{{c.rx, c.ry, 1.0}, {c.gx, c.gy, 1.0}, {c.bx, c.by, 1.0}};
        cmsToneCurve *power = cmsBuildGamma(nullptr, 2.2);
        cmsToneCurve *curves[3] = {power, power, power};
        profile = cmsCreateRGBProfile(&white, &primaries, curves);
        cmsFreeToneCurve(power);
        if (!profile) {
            *error = QStringLiteral("lcms2 could not build a base profile");
            return false;
        }
    }

    const cmsTagSignature trcTags[3] = {cmsSigRedTRCTag, cmsSigGreenTRCTag, cmsSigBlueTRCTag};
    for (cmsTagSignature tag : trcTags) {
        const auto *base = static_cast<const cmsToneCurve *>(cmsReadTag(profile, tag));
        if (!base) {
            cmsCloseProfile(profile);
            *error = QStringLiteral("profile has no TRC tags (LUT-based profiles are not supported)");
            return false;
        }
        cmsToneCurve *raised = raisedCurve(base, gamma);
        const bool ok = raised && cmsWriteTag(profile, tag, raised);
        cmsFreeToneCurve(raised);
        if (!ok) {
            cmsCloseProfile(profile);
            *error = QStringLiteral("could not write tone curve");
            return false;
        }
    }

    cmsMLU *desc = cmsMLUalloc(nullptr, 1);
    cmsMLUsetASCII(desc, "en", "US", QStringLiteral("plasma-gamma %1").arg(gamma, 0, 'f', 2).toLatin1().constData());
    cmsWriteTag(profile, cmsSigProfileDescriptionTag, desc);
    cmsMLUfree(desc);

    QDir().mkpath(QFileInfo(path).absolutePath());
    const bool saved = cmsSaveProfileToFile(profile, QFile::encodeName(path).constData());
    cmsCloseProfile(profile);
    if (!saved) {
        *error = QStringLiteral("cannot write %1").arg(path);
    }
    return saved;
}

}
