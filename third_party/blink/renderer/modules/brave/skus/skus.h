// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_SKUS_SKUS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_SKUS_SKUS_H_

#include "brave/components/skus/skus.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class ScriptPromise;
class ScriptState;

class MODULES_EXPORT Skus final
    : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  Skus() = default;
  ~Skus() override;
  ScriptPromise refresh_order(ScriptState*, uint32_t);
  ScriptPromise fetch_order_credentials(ScriptState*, uint32_t);

  void Trace(blink::Visitor*) const override;

 private:
  // Used to call the Rust SDK (in a different process).
  mojo::Remote<brave_rewards::SkusSdkCallerImpl> skus_sdk_caller_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_SKUS_SKUS_H_
