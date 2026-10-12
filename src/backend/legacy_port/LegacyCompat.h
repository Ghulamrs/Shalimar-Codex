#ifndef SHALIMAR_LEGACY_COMPAT_H
#define SHALIMAR_LEGACY_COMPAT_H
#include <sstream>
#include <string>
#include <stdint.h>
namespace shalimar { template <typename T> std::string legacyToString(const T &v) { std::ostringstream out; out << v; return out.str(); } }
#endif
