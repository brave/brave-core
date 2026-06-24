// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// snap_host.ts — Entry point for chrome://wallet-snap-host/
//
// Mirrors the wallet_page_api_proxy.ts pattern: binds WalletSnapHostHandlerFactory
// via getRemote() (auto-bound by MojoWebUIController), then wires SnapBridge,
// SnapRequestHandler, and SnapsService together.

import { BraveWallet } from '../constants/types'
import { SnapBridge } from '../common/snap/snap_bridge'

const snapBridge = new SnapBridge()
const snapRequestHandler = new BraveWallet.SnapRequestHandlerRemote()
const snapsService = new BraveWallet.SnapsServiceRemote()

const factory = BraveWallet.WalletSnapHostHandlerFactory?.getRemote?.()
factory?.createSnapHostHandler?.(
  snapBridge.bindNewPipeAndPassRemote(),
  snapRequestHandler.$.bindNewPipeAndPassReceiver(),
  snapsService.$.bindNewPipeAndPassReceiver(),
)

snapBridge.setSnapRequestHandler(snapRequestHandler)
snapBridge.setSnapsService(snapsService)
