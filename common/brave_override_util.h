/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_BRAVE_OVERRIDE_UTIL_H_
#define BRAVE_COMMON_BRAVE_OVERRIDE_UTIL_H_

// Copied from https://stackoverflow.com/a/27490954
constexpr bool strings_equal(char const * a, char const * b) {
    return *a == *b && (*a == '\0' || strings_equal(a + 1, b + 1));
}

#endif  // BRAVE_COMMON_BRAVE_OVERRIDE_UTIL_H_