/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constant
import { types } from '../constants/rewards_types'

export const createWallet = () => action(types.CREATE_WALLET)

export const onWalletCreated = () => action(types.WALLET_CREATED)

export const onWalletCreateFailed = () => action(types.WALLET_CREATE_FAILED)

export const onSettingSave = (key: string, value: any) => action(types.ON_SETTING_SAVE, {
  key,
  value
})
