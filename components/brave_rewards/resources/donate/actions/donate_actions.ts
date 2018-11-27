/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constant
import { types } from '../constants/donate_types'

export const onCloseDialog = () => action(types.ON_CLOSE_DIALOG)

export const onPublisherBanner = (data: RewardsDonate.Publisher) => action(types.ON_PUBLISHER_BANNER, {
  data
})

export const getWalletProperties = () => action(types.GET_WALLET_PROPERTIES)

export const onWalletProperties = (properties: {status: number, wallet: RewardsDonate.WalletProperties}) =>
  action(types.ON_WALLET_PROPERTIES, {
    properties
  })

export const onDonate = (publisherKey: string, amount: number, recurring: boolean) => action(types.ON_DONATE, {
  publisherKey,
  amount,
  recurring
})

export const getRecurringDonations = () => action(types.GET_RECURRING_DONATIONS)

export const onRecurringDonations = (list: string[]) => action(types.ON_RECURRING_DONATIONS, {
  list
})
