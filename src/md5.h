/**
 * MD5 hashing utilities
 *
 * Provided for interoperating with third-party web APIs that still sign
 * requests with MD5 (e.g. the Game Jolt Game API). Do not use MD5 for anything
 * security-sensitive.
 */

#pragma once

#include <string>
#include <string_view>

namespace rexgamejolt {

// Returns the 32-character lowercase hex digest of `data`.
std::string Md5(std::string_view data);

}  // namespace rexgamejolt
