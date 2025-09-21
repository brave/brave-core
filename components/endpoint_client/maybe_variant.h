/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_VARIANT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_VARIANT_H_

#include <type_traits>
#include <variant>

namespace endpoints::detail {

// Primary template: would only match the empty parameter pack,
// which is excluded by the alias' requires-clause below.
template <typename...>
struct MaybeVariantImpl;

// Partial specialization: a single type maps to itself.
template <typename T>
struct MaybeVariantImpl<T> : std::type_identity<T> {};

// Partial specialization: two or more types map to std::variant<>.
template <typename... Ts>
  requires(sizeof...(Ts) > 1)
struct MaybeVariantImpl<Ts...> : std::type_identity<std::variant<Ts...>> {};

// Alias: maps
// - sizeof...(Ts) == 1 to the single type,
// - sizeof...(Ts) > 1 to std::variant<Ts...>,
// Instantiation with an empty pack is blocked by the requires-clause.
template <typename... Ts>
  requires(sizeof...(Ts) > 0)
using MaybeVariant = typename MaybeVariantImpl<Ts...>::type;

}  // namespace endpoints::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_VARIANT_H_
