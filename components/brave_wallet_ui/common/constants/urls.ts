// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { loadTimeData } from '../../../common/loadTimeData'

export const FILECOIN_FORMAT_DESCRIPTION_URL = 'https://lotus.filecoin.io/lotus/manage/lotus-cli/#lotus-wallet-import'

const braveWalletImageBridgeUrl = loadTimeData.getString('braveWalletImageBridgeUrl')

export const IMAGE_BRIDGE_URL = braveWalletImageBridgeUrl.endsWith('/')
  ? braveWalletImageBridgeUrl.slice(0, -1) // Strip off trailing '/' in URL
  : braveWalletImageBridgeUrl
