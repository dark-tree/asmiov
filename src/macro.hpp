#pragma once

#define JOIN_PARTS_RESOLVED(first, second) first##second

/// Allows one to merge two tokens, even macro values
#define JOIN_PARTS(first, second) JOIN_PARTS_RESOLVED(first, second)

/// Mark that no padding should be used in marked struct, ever
#define PACKED __attribute__((__packed__))