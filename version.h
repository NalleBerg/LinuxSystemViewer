// Single place to change the application version string
#pragma once

// Use a simple macro so it can be used in literal concatenation in other
// translation units where convenient. Change this to bump the version.
#define LSV_VERSION "0.6.5"

// Helper to get a QString version when needed. Include <QString> here so
// consumers that include this header get the helper without needing to
// include Qt headers themselves first.
#include <QString>
static inline QString LSVVersionQString()
{
    return QString::fromUtf8(LSV_VERSION);
}
