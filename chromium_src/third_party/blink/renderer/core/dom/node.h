/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_

#include <type_traits>

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"

#define MarkAncestorsWithChildNeedsStyleInvalidation             \
  NotUsed();                                                     \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, void NodeConstructed();) \
  void MarkAncestorsWithChildNeedsStyleInvalidation

#include "src/third_party/blink/renderer/core/dom/node.h"  // IWYU pragma: export

#undef MarkAncestorsWithChildNeedsStyleInvalidation

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
namespace blink {
class ActiveScriptWrappableBase;
}

namespace cppgc {

template <typename T, typename>
struct PostConstructionCallbackTrait;

// This PostConstruction extension adds Node::NodeConstructed() call after the
// Node construction. We use this to track fully constructed Node in the
// PageGraph engine. It's important to have all subclasses constructed so we can
// get the actual Node type and do DynamicTo<>() conversion if required.
template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<
        std::is_base_of<blink::Node, T>::value &&
            !HasActiveScriptWrappableBaseConstructed<T>::value,
        void>::type> {
  static void Call(blink::Node* object) { object->NodeConstructed(); }
};

// If Node is derived from ActiveScriptWrappable<> we need to call both
// PostConstructionCallbacks.
template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<
        std::is_base_of<blink::Node, T>::value &&
            HasActiveScriptWrappableBaseConstructed<T>::value,
        void>::type> {
  template <typename U>
  struct class_of {};

  template <typename U, typename R>
  struct class_of<R(U::*)> {
    using type = U;
  };

  static void Call(T* object) {
    // Ensure we use a proper ActiveScriptWrappable<> post construction trait.
    using ActiveScriptWrappableType = typename class_of<
        decltype(&T::ActiveScriptWrappableBaseConstructed)>::type;
    static_assert(
        std::is_base_of<blink::ActiveScriptWrappableBase,
                        ActiveScriptWrappableType>::value &&
        !std::is_base_of<blink::Node, ActiveScriptWrappableType>::value);
    PostConstructionCallbackTrait<ActiveScriptWrappableType>::Call(object);

    PostConstructionCallbackTrait<blink::Node>::Call(object);
  }
};

}  // namespace cppgc
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_
