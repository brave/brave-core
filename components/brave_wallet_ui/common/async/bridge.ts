// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../wallet_api_proxy'
import getWalletPanelApiProxy from '../../panel/wallet_panel_api_proxy'
import getWalletPageApiProxy from '../../page/wallet_page_api_proxy'

export function getAPIProxy (): WalletApiProxy {
  return window.location.hostname === 'wallet-panel.top-chrome'
    ? getWalletPanelApiProxy() : getWalletPageApiProxy()
}

export default getAPIProxy
