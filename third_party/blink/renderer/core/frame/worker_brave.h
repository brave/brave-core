// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_WORKER_BRAVE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_WORKER_BRAVE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class ScriptPromise;
class ScriptState;

class CORE_EXPORT WorkerBrave : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ScriptPromise isBrave(ScriptState*);

  WorkerBrave() = default;

  void Trace(Visitor*) const override;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_WORKER_BRAVE_H_
