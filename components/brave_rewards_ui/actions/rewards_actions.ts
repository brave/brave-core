/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'
import { types } from '../constants/rewards_types'

export const createWalletRequested = () => action(types.CREATE_WALLET_REQUESTED)

export const walletCreated = () => action(types.WALLET_CREATED)

export const walletCreateFailed = () => action(types.WALLET_CREATE_FAILED)
