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

export const getRecurringTips = () => action(types.GET_RECURRING_TIPS)

export const onRecurringTips = (list: RewardsDonate.RecurringTips[]) => action(types.ON_RECURRING_TIPS, {
  list
})

export const getReconcileStamp = () => action(types.GET_RECONCILE_STAMP)

export const onReconcileStamp = (stamp: number) => action(types.ON_RECONCILE_STAMP, {
  stamp
})

export const onRecurringTipRemoved = (success: boolean) => action(types.ON_RECURRING_TIP_REMOVED, {
  success
})
