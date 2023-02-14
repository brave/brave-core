// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_NAVIGATOR_BRAVE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_NAVIGATOR_BRAVE_H_

#include "third_party/blink/renderer/platform/bindings/name_client.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Brave;
class NavigatorBase;

class NavigatorBrave final : public GarbageCollected<NavigatorBrave>,
                             public Supplement<NavigatorBase>,
                             public NameClient {
 public:
  static const char kSupplementName[];

  static NavigatorBrave& From(NavigatorBase&);
  static Brave* brave(NavigatorBase&);
  Brave* brave();

  explicit NavigatorBrave(NavigatorBase&);

  void Trace(blink::Visitor*) const override;
  const char* NameInHeapSnapshot() const override { return "NavigatorBrave"; }

 private:
  Member<Brave> brave_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_NAVIGATOR_BRAVE_H_
