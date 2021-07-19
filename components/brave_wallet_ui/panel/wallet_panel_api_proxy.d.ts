// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { WalletAPIHandler } from '../constants/types'

export default class APIProxy {
  static getInstance: () => APIProxy
  getWalletHandler: () => WalletAPIHandler
  getCallbackRouter: () => CallbackRouter
  showUI: () => {}
  closeUI: () => {}
  connectToSite: (accounts: string[], origin: string, tabId: number) => {}
  cancelConnectToSite: (origin: string, tabId: number) => {}
}
