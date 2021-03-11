// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_ETHEREUM_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_ETHEREUM_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class ExecutionContext;
class ScriptPromise;
class ScriptState;

class MODULES_EXPORT Ethereum final
    : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  Ethereum() = default;
  ScriptPromise request(ScriptState*, const String& input);

 private:
  bool EnsureConnected(ExecutionContext* execution_context);

  mojo::Remote<brave_wallet::mojom::BraveWalletProvider> brave_wallet_provider_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_BRAVE_ETHEREUM_H_
