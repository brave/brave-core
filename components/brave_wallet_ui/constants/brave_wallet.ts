// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Aggregates the generated brave wallet mojom modules into a single module, so
// that consumers can `import * as BraveWallet from '.../brave_wallet'` without
// having to know which mojom file a definition lives in.
export * from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
export * from 'gen/brave/components/brave_wallet/common/mojom/brave_wallet_common.mojom.m.js'
