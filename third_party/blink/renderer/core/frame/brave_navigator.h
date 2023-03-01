/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_BRAVE_NAVIGATOR_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_BRAVE_NAVIGATOR_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {

class WorkerBrave;

class CORE_EXPORT BraveNavigator : public GarbageCollectedMixin {
 public:
  BraveNavigator() = default;

  WorkerBrave* brave();

  void Trace(blink::Visitor*) const override;

 private:
  Member<WorkerBrave> brave_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_BRAVE_NAVIGATOR_H_
