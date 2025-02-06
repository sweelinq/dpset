#pragma once

#include <QString>

enum class Orientation {
    Normal,
    Inverted,
    Left,
    Right
};

inline QString orientationToString(Orientation orient)
{
    switch (orient) {
    case Orientation::Normal:   return "normal";
    case Orientation::Inverted: return "inverted";
    case Orientation::Left:     return "left";
    case Orientation::Right:    return "right";
    }
    return "normal"; // fallback
}

inline Orientation stringToOrientation(const QString &str)
{
    QString s = str.toLower();
    if (s == "normal")
        return Orientation::Normal;
    if (s.contains("left"))
        return Orientation::Left;
    if (s.contains("inverted"))
        return Orientation::Inverted;
    if (s.contains("right"))
        return Orientation::Right;
    return Orientation::Normal;
}
