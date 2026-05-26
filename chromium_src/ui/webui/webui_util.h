/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_WEBUI_WEBUI_UTIL_H_
#define BRAVE_CHROMIUM_SRC_UI_WEBUI_WEBUI_UTIL_H_

#include <array>
#include <string_view>

#include "base/containers/span.h"

// Replace the upstream kDefaultTrustedTypesPolicies with a Brave-augmented
// version so that every WebUI source that derives its CSP from this constant
// (directly or via base::StrCat) automatically permits the Brave-specific
// policies.
//
// Without this, any UI which overrides the CSP will be missing our policies.
#define kDefaultTrustedTypesPolicies kDefaultTrustedTypesPolicies_ChromiumImpl

#include <ui/webui/webui_util.h>  // IWYU pragma: export

#undef kDefaultTrustedTypesPolicies

namespace webui {

namespace internal {

// Compile-time concatenation of two null-terminated string literals into a
// std::array, dropping the first literal's null terminator. The inputs are
// wrapped in base::span so the indexing satisfies -Wunsafe-buffer-usage.
template <size_t N1, size_t N2>
constexpr std::array<char, N1 + N2 - 1> ConcatStringLiterals(
    const char (&a)[N1],
    const char (&b)[N2]) {
  std::array<char, N1 + N2 - 1> result{};
  base::span<const char, N1> a_span(a);
  base::span<const char, N2> b_span(b);

  // Note: We drop the null terminator from the first string.
  for (size_t i = 0; i + 1 < N1; ++i) {
    result[i] = a_span[i];
  }
  for (size_t i = 0; i < N2; ++i) {
    result[N1 - 1 + i] = b_span[i];
  }
  return result;
}

inline constexpr auto kBraveTrustedTypesPoliciesStorage =
    ConcatStringLiterals(kDefaultTrustedTypesPolicies_ChromiumImpl,
                         " svelte-trusted-html default");

}  // namespace internal

// Augments the upstream policy list with the Trusted Types policies registered
// by @brave/leo's Svelte runtime (`svelte-trusted-html`) and our `default`
// policy.
inline constexpr std::string_view kDefaultTrustedTypesPolicies(
    internal::kBraveTrustedTypesPoliciesStorage.data(),
    internal::kBraveTrustedTypesPoliciesStorage.size() - 1);

}  // namespace webui

#endif  // BRAVE_CHROMIUM_SRC_UI_WEBUI_WEBUI_UTIL_H_
