// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_BRAVE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_BRAVE_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/execution_context/navigator_base.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class ScriptState;

class MODULES_EXPORT Brave final : public ScriptWrappable,
                                   public Supplement<NavigatorBase> {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static const char kSupplementName[];

  static Brave* brave(NavigatorBase& navigator);

  explicit Brave(NavigatorBase& navigator);
  ~Brave() override = default;

  void Trace(Visitor*) const override;

  ScriptPromise<IDLBoolean> isBrave(ScriptState*);
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_BRAVE_H_
