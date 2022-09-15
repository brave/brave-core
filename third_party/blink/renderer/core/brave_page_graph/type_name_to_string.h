/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_TYPE_NAME_TO_STRING_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_TYPE_NAME_TO_STRING_H_

#include <array>
#include <string>
#include <utility>

#include "base/strings/string_piece.h"

namespace blink {
namespace internal {

// https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
template <std::size_t... Idxs>
constexpr auto substring_as_array(base::StringPiece str,
                                  std::index_sequence<Idxs...>) {
  return std::array{str[Idxs]..., '\n'};
}

template <typename T>
constexpr auto type_name_array() {
#if defined(__clang__)
  constexpr auto prefix = base::StringPiece{"[T = "};
  constexpr auto suffix = base::StringPiece{"]"};
  constexpr auto function = base::StringPiece{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
  constexpr auto prefix = base::StringPiece{"with T = "};
  constexpr auto suffix = base::StringPiece{"]"};
  constexpr auto function = base::StringPiece{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
  constexpr auto prefix = base::StringPiece{"type_name_array<"};
  constexpr auto suffix = base::StringPiece{">(void)"};
  constexpr auto function = base::StringPiece{__FUNCSIG__};
#else
#error Unsupported compiler
#endif

  constexpr auto start = function.find(prefix) + prefix.size();
  constexpr auto end = function.rfind(suffix);

  static_assert(start < end);

  constexpr auto name = function.substr(start, (end - start));
  return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
  static inline constexpr auto value = type_name_array<T>();
};

}  // namespace internal

template <typename T>
constexpr base::StringPiece type_name_to_string() {
  constexpr auto& value = internal::type_name_holder<T>::value;
  return base::StringPiece(value.data(), value.size());
}

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_TYPE_NAME_TO_STRING_H_
