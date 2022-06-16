/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_

#include <type_traits>

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/platform/bindings/active_script_wrappable_base.h"

#define MarkAncestorsWithChildNeedsStyleInvalidation             \
  NotUsed();                                                     \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, void NodeConstructed();) \
  void MarkAncestorsWithChildNeedsStyleInvalidation

#include "src/third_party/blink/renderer/core/dom/node.h"

#undef MarkAncestorsWithChildNeedsStyleInvalidation

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
namespace blink {
class ActiveScriptWrappableBase;
}

namespace cppgc {

template <typename T, typename>
struct PostConstructionCallbackTrait;

template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<
        std::is_base_of<blink::Node, T>::value &&
            !std::is_base_of<blink::ActiveScriptWrappableBase, T>::value,
        void>::type> {
  static void Call(blink::Node* object) { object->NodeConstructed(); }
};

template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<
        std::is_base_of<blink::Node, T>::value &&
            std::is_base_of<blink::ActiveScriptWrappableBase, T>::value,
        void>::type> {
  static void Call(T* object) {
    PostConstructionCallbackTrait<blink::ActiveScriptWrappableBase>::Call(
        object);
    PostConstructionCallbackTrait<blink::Node>::Call(object);
  }
};

}  // namespace cppgc
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_H_
