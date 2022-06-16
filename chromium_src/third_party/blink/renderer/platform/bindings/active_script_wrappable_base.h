/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_ACTIVE_SCRIPT_WRAPPABLE_BASE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_ACTIVE_SCRIPT_WRAPPABLE_BASE_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8.h"

#define PostConstructionCallbackTrait PostConstructionCallbackTrait_ChromiumImpl

#include "src/third_party/blink/renderer/platform/bindings/active_script_wrappable_base.h"

#undef PostConstructionCallbackTrait

namespace blink {
class Node;
}  // namespace blink

namespace cppgc {

template <typename T, typename>
struct PostConstructionCallbackTrait;

template <typename T>
struct PostConstructionCallbackTrait<
    T,
    typename std::enable_if<
        std::is_base_of<blink::ActiveScriptWrappableBase, T>::value &&
            !std::is_base_of<blink::Node, T>::value,
        void>::type> {
  static void Call(T* object) {
    PostConstructionCallbackTrait_ChromiumImpl<T, void>::Call(object);
  }
};

}  // namespace cppgc

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_ACTIVE_SCRIPT_WRAPPABLE_BASE_H_
