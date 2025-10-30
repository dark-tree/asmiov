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
#include <functional>
#include <iostream>
#include <algorithm>
#include <limits>

// systems
#ifdef __linux__
#	include <unistd.h>
#	include <fcntl.h>
#	include <sys/types.h>
#	include <sys/mman.h>
#	include <sys/wait.h>
#else
#	error "Non-linux platforms not yet suported!"
#endif

// architectures
#if defined(__x86_64__) || defined(_M_X64)
#	define ARCH_X86 true
#else
#	define ARCH_X86 false
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#	define ARCH_AARCH64 true
#else
#	define ARCH_AARCH64 false
#endif

#if !(ARCH_X86 || ARCH_AARCH64)
#	error "Unsupported target architecture!"
#endif