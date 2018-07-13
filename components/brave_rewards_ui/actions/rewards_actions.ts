/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const types = require('../constants/rewards_types')

export const createWalletRequested = () => ({
  type: types.CREATE_WALLET_REQUESTED
})

export const walletCreated = () => ({
  type: types.WALLET_CREATED
})

export const walletCreateFailed = () => ({
  type: types.WALLET_CREATE_FAILED
})
