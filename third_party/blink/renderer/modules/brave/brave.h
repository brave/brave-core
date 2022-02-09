// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_BRAVE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_BRAVE_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class ScriptPromise;
class ScriptState;

class MODULES_EXPORT Brave final
    : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  Brave() = default;
  ScriptPromise isBrave(ScriptState*);
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_BRAVE_H_
