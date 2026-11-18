// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { PolkadotBridgeUIHandler } from 'gen/brave/components/brave_wallet/common/polkadot_bridge.mojom.m.js'

import { TypeRegistry, Metadata, GenericChainProperties } from '@polkadot/types'
import { allExtensions } from '@polkadot/types/extrinsic/signedExtensions'

const uiHandler = PolkadotBridgeUIHandler.getRemote()
console.log(uiHandler)
console.log(TypeRegistry, Metadata, GenericChainProperties, allExtensions)
