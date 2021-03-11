// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_WINDOW_BRAVE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_WINDOW_BRAVE_H_

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/platform/bindings/name_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Ethereum;

class WindowBrave final
    : public GarbageCollected<WindowBrave>,
      public Supplement<LocalDOMWindow>,
      public NameClient {

 public:
  static const char kSupplementName[];

  static WindowBrave& From(LocalDOMWindow&);
  static Ethereum* ethereum(LocalDOMWindow&);
  Ethereum* ethereum();

  explicit WindowBrave(LocalDOMWindow&);

  void Trace(blink::Visitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "WindowBrave";
  }

 private:
  Member<Ethereum> ethereum_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_WINDOW_BRAVE_H_
