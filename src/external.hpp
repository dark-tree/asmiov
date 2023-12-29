#pragma once

// C
#include <cinttypes>
#include <cstring>

// C++
#include <stdexcept>
#include <bit>
#include <bitset>
#include <iomanip>
#include <utility>
#include <vector>
#include <unordered_map>

#ifdef __linux__
#	include <sys/mman.h>
#	include <unistd.h>
#else
#	error "Other platforms not yet suported!"
#endif
