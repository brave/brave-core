/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_BLINK_CONVERTERS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_BLINK_CONVERTERS_H_

#include <sstream>
#include <string>
#include <type_traits>

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/type_name_to_string.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_related_application.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptValue;
class TextMetrics;
class CanvasRenderingContext;

namespace internal {

// https://gist.github.com/szymek156/9b1b90fe474277be4641e9ef4666f472
template <typename Class>
struct has_ostream_operator_impl {
  template <typename V>
  static auto test(V*)  // NOLINT
      -> decltype(std::declval<std::ostream>() << std::declval<V>());
  template <typename>
  static auto test(...) -> std::false_type;

  using type = typename std::is_same<std::ostream&,
                                     decltype(test<Class>(nullptr))>::type;
};

template <typename Class>
struct has_ostream_operator : has_ostream_operator_impl<Class>::type {};

template <typename T, typename = void>
struct is_iterable : std::false_type {};

// this gets used only when we can call std::begin() and std::end() on that type
template <typename T>
struct is_iterable<T,
                   std::void_t<decltype(std::begin(std::declval<T>())),
                               decltype(std::end(std::declval<T>()))>>
    : std::true_type {};

}  // namespace internal

// Override to convert WebAPI call argument to String value.
template <typename T>
typename std::enable_if<!internal::has_ostream_operator<T>::value &&
                            !internal::is_iterable<T>::value,
                        String>::type
ToPageGraphBlinkArg(const T&) {
#if DCHECK_IS_ON()
  std::stringstream result;
  result << "[" << type_name_to_string<T>() << "]";
  return String(result.str());
#else   // DCHECK_IS_ON()
  return String();
#endif  // DCHECK_IS_ON()
}

// Override to pass additional blink receiver class data during WebAPI call.
template <typename T>
PageGraphBlinkReceiverData ToPageGraphBlinkReceiverData(T*) {
  return {};
}

// ostream-supported handler.
template <typename T>
typename std::enable_if<internal::has_ostream_operator<T>::value &&
                            !internal::is_iterable<T>::value,
                        String>::type
ToPageGraphBlinkArg(const T& value) {
  std::stringstream result;
  result << value;
  return String(result.str());
}

template <typename T>
typename std::enable_if<!internal::has_ostream_operator<T>::value &&
                            internal::is_iterable<T>::value,
                        String>::type
ToPageGraphBlinkArg(const T& values) {
  std::stringstream result;
  result << "{";
  bool added = false;
  for (const auto& value : values) {
    if (added) {
      result << ", ";
    }
    result << ToPageGraphBlinkArg(value);
    added = true;
  }
  result << "}";
  return String(result.str());
}

template <>
inline String ToPageGraphBlinkArg(const String& string) {
  return string;
}

template <>
inline String ToPageGraphBlinkArg(const AtomicString& string) {
  return string.GetString();
}

template <>
CORE_EXPORT String
ToPageGraphBlinkArg(const bindings::NativeValueTraitsStringAdapter& adapter);
template <>
CORE_EXPORT String ToPageGraphBlinkArg(const ScriptValue& script_value);

CORE_EXPORT String ToPageGraphBlinkArg(TextMetrics* result);
CORE_EXPORT String ToPageGraphBlinkArg(CanvasRenderingContext* context);

CORE_EXPORT PageGraphBlinkReceiverData
ToPageGraphBlinkReceiverData(Document* document);

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_BLINK_CONVERTERS_H_
