/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_ACTIVE_SCRIPT_WRAPPABLE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_ACTIVE_SCRIPT_WRAPPABLE_H_

#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/active_script_wrappable_base.h"

#define PostConstructionCallbackTrait PostConstructionCallbackTrait_ChromiumImpl
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"  // IWYU pragma: export

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef PostConstructionCallbackTrait

namespace blink {
class Node;
}  // namespace blink

namespace cppgc {

template <typename T, typename>
struct PostConstructionCallbackTrait;

template <class, class = void>
struct HasActiveScriptWrappableBaseConstructed : std::false_type {};

template <class T>
struct HasActiveScriptWrappableBaseConstructed<
    T,
    std::void_t<
        decltype(std::declval<T>().ActiveScriptWrappableBaseConstructed())>>
    : std::true_type {};

// Allow PostConstructionCallback to work with Node types.
template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<HasActiveScriptWrappableBaseConstructed<T>::value &&
                                !std::is_base_of<blink::Node, T>::value,
                            void>::type> {
  static void Call(T* object) {
    PostConstructionCallbackTrait_ChromiumImpl<T, void>::Call(object);
  }
};

}  // namespace cppgc
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_ACTIVE_SCRIPT_WRAPPABLE_H_
