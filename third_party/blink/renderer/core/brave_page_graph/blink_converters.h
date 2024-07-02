/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_BLINK_CONVERTERS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_BLINK_CONVERTERS_H_

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/auto_reset.h"
#include "base/containers/span.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_handwriting_recognizer_query_result.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_related_application.h"  // IWYU pragma: keep
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace pg_internal {

template <typename T>
using strip_type_qualifiers_t =
    std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<T>>>;

template <typename T>
concept has_tov8 =
    std::is_same_v<decltype(ToV8Traits<strip_type_qualifiers_t<T>>::ToV8(
                       std::declval<ScriptState*>(),
                       std::declval<T&>())),
                   v8::Local<v8::Value>>;

template <typename T>
concept is_iterable = requires(T t) {
  { std::begin(t) } -> std::same_as<decltype(std::begin(t))>;
  { std::end(t) } -> std::same_as<decltype(std::end(t))>;
};

template <typename T>
concept convert_as_base_value =
    requires(T a) {
      { base::Value(a) } -> std::same_as<base::Value>;
    } || std::is_same_v<T, uint32_t> || std::is_same_v<T, int64_t> ||
    std::is_same_v<T, uint64_t>;

template <typename T>
concept convert_as_string =
    !convert_as_base_value<T> && std::is_convertible_v<T, String>;

template <typename T>
concept convert_via_tov8 =
    !convert_as_base_value<T> && !convert_as_string<T> && has_tov8<T>;

template <typename T>
concept convert_via_tov8_ref =
    !convert_as_base_value<T> && !convert_as_string<T> && !has_tov8<T> &&
    has_tov8<std::add_pointer_t<T>>;

template <typename T>
concept convert_as_iterable =
    !convert_as_base_value<T> && !convert_as_string<T> && !has_tov8<T> &&
    !has_tov8<std::add_pointer_t<T>> && is_iterable<T>;

template <typename T>
concept convert_as_enum_value =
    !convert_as_base_value<T> && !convert_as_string<T> && !has_tov8<T> &&
    !has_tov8<std::add_pointer_t<T>> && !is_iterable<T> && std::is_enum_v<T>;

template <typename T>
concept convert_unsupported =
    !convert_as_base_value<T> && !convert_as_string<T> && !has_tov8<T> &&
    !has_tov8<std::add_pointer_t<T>> && !is_iterable<T> && !std::is_enum_v<T>;

}  // namespace pg_internal

// This intentially left unimplemented to catch all unsupported types as linker
// errors.
template <typename T>
  requires(pg_internal::convert_unsupported<T>)
base::Value ToPageGraphValue(ScriptState* script_state, const T& value);

// Convert basic types and integers that fit into double.
template <typename T>
  requires(pg_internal::convert_as_base_value<T>)
base::Value ToPageGraphValue(ScriptState* script_state, const T& value) {
  if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, int64_t> ||
                std::is_same_v<T, uint64_t>) {
    return base::Value(static_cast<double>(value));
  } else {
    return base::Value(value);
  }
}

// Convert String-convertible types.
template <typename T>
  requires(pg_internal::convert_as_string<T>)
base::Value ToPageGraphValue(ScriptState* script_state, const T& value) {
  return base::Value(String(value).Utf8());
}

// Convert types that have ToV8Traits.
template <typename T>
  requires(pg_internal::convert_via_tov8<T>)
base::Value ToPageGraphValue(ScriptState* script_state, T& value) {
  if constexpr (std::is_pointer_v<T>) {
    if (!value) {
      return base::Value();
    }
  }
  v8::Local<v8::Value> v8_value =
      ToV8Traits<pg_internal::strip_type_qualifiers_t<T>>::ToV8(script_state,
                                                                value);
  return ToPageGraphValue(script_state, v8_value);
}

// Convert types that have ToV8Traits with reference.
template <typename T>
  requires(pg_internal::convert_via_tov8_ref<T>)
base::Value ToPageGraphValue(ScriptState* script_state, T& value) {
  if constexpr (std::is_pointer_v<T>) {
    if (!value) {
      return base::Value();
    }
  }
  v8::Local<v8::Value> v8_value =
      ToV8Traits<pg_internal::strip_type_qualifiers_t<T>>::ToV8(script_state,
                                                                &value);
  return ToPageGraphValue(script_state, v8_value);
}

// Convert iterable types.
template <typename T>
  requires(pg_internal::convert_as_iterable<T>)
base::Value ToPageGraphValue(ScriptState* script_state, const T& values) {
  base::Value::List list_values;
  for (const auto& value : values) {
    list_values.Append(ToPageGraphValue(script_state, value));
  }
  return base::Value(std::move(list_values));
}

// Helper for tuple types converter.
template <typename... Ts, std::size_t... Is>
void ToPageGraphValueImpl(ScriptState* script_state,
                          base::Value::List& list_values,
                          const std::tuple<Ts...>& values,
                          std::index_sequence<Is...>) {
  (list_values.Append(ToPageGraphValue(script_state, std::get<Is>(values))),
   ...);
}

// Convert tuple types.
template <typename... Ts>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const std::tuple<Ts...>& values) {
  base::Value::List list_values;
  ToPageGraphValueImpl(script_state, list_values, values,
                       std::index_sequence_for<Ts...>{});
  return base::Value(std::move(list_values));
}

// Convert enum types.
template <typename T>
  requires(pg_internal::convert_as_enum_value<T>)
base::Value ToPageGraphValue(ScriptState* script_state, const T& value) {
  return base::Value(static_cast<std::underlying_type_t<T>>(value));
}

// Convert std::pair types.
template <typename T1, typename T2>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const std::pair<T1, T2>& values) {
  return ToPageGraphValue(script_state, std::tie(values.first, values.second));
}

// Convert std::optional<T> types.
template <typename T>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const std::optional<T>& opt_value) {
  if (!opt_value) {
    return base::Value();
  }
  return ToPageGraphValue(script_state, opt_value.value());
}

// Convert ScriptPromise<T> types.
template <typename T>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const ScriptPromise<T>& script_promise) {
  return ToPageGraphValue<ScriptPromiseUntyped>(script_state, script_promise);
}

// Convert v8::Value.
template <>
CORE_EXPORT base::Value ToPageGraphValue(ScriptState* script_state,
                                         const v8::Local<v8::Value>& arg);

// Convert ScriptValue.
template <>
CORE_EXPORT base::Value ToPageGraphValue(ScriptState* script_state,
                                         const ScriptValue& script_value);

// Convert NativeValueTraitsAnyAdapter types.
template <>
CORE_EXPORT base::Value ToPageGraphValue<bindings::NativeValueTraitsAnyAdapter>(
    ScriptState* script_state,
    const bindings::NativeValueTraitsAnyAdapter& adapter);

// Convert ScriptPromiseUntyped.
template <>
CORE_EXPORT base::Value ToPageGraphValue<ScriptPromiseUntyped>(
    ScriptState* script_state,
    const ScriptPromiseUntyped& script_promise);

// Convert EventListener.
template <>
CORE_EXPORT base::Value ToPageGraphValue(
    ScriptState* script_state,
    blink::EventListener* const& event_listener);

// Override to pass additional blink data during WebAPI call.
template <typename T>
PageGraphObject ToPageGraphObject(T*) {
  return {};
}

template <>
CORE_EXPORT PageGraphObject ToPageGraphObject(Document* document);

// Scoped helper to not trigger nested PG calls during v8 attribute resolution.
CORE_EXPORT std::optional<base::AutoReset<bool>> ScopedPageGraphCall();

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_BLINK_CONVERTERS_H_
