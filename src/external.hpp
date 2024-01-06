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
#include <optional>
#include <list>

#ifdef __linux__
#	include <unistd.h>
#	include <fcntl.h>
#	include <sys/types.h>
#	include <sys/mman.h>
#	include <sys/wait.h>
#else
#	error "Other platforms not yet suported!"
#endif
