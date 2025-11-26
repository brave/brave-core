/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import { RewardsPageProxy } from '../../../../components/brave_rewards/resources/rewards_page/webui/rewards_page_proxy'
import { externalWalletFromExtensionData } from '../../../../components/brave_rewards/resources/shared/lib/external_wallet'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounce } from '$web-common/debounce'
import {
  RewardsState,
  RewardsActions,
  defaultRewardsActions,
} from './rewards_state'

export function createRewardsHandler(
  store: Store<RewardsState>,
): RewardsActions {
  if (!loadTimeData.getBoolean('rewardsFeatureEnabled')) {
    store.update({ initialized: true })
    return defaultRewardsActions()
  }

  const newTabProxy = NewTabPageProxy.getInstance()
  const newTabHandler = newTabProxy.handler
  const rewardsProxy = RewardsPageProxy.getInstance()
  const rewardsHandler = rewardsProxy.handler

  store.update({ rewardsFeatureEnabled: true })

  async function updatePrefs() {
    const { showRewardsWidget } = await newTabHandler.getShowRewardsWidget()
    store.update({ showRewardsWidget })
  }

  async function updateParameters() {
    const { rewardsParameters } = await rewardsHandler.getRewardsParameters()
    if (rewardsParameters) {
      store.update({
        rewardsExchangeRate: rewardsParameters.rate,
        payoutStatus: rewardsParameters.payoutStatus,
      })
    }
  }

  async function updateRewardsEnabled() {
    const { paymentId } = await rewardsHandler.getRewardsPaymentId()
    store.update({ rewardsEnabled: Boolean(paymentId) })
  }

  async function updateExternalWallet() {
    const { externalWallet } = await rewardsHandler.getExternalWallet()
    store.update({
      rewardsExternalWallet: externalWalletFromExtensionData(externalWallet),
    })
  }

  async function updateBalance() {
    const { balance } = await rewardsHandler.getAvailableBalance()
    store.update({ rewardsBalance: balance })
  }

  async function updateAdsData() {
    const { statement } = await rewardsHandler.getAdsStatement()
    if (statement) {
      let rewardsAdsViewed = 0
      Object.values(statement.adTypeSummaryThisMonth).map((value) => {
        if (typeof value === 'number') {
          rewardsAdsViewed += value
        }
      })
      store.update({
        rewardsAdsViewed,
        minEarningsPreviousMonth: statement.minEarningsPreviousMonth,
      })
    } else {
      store.update({
        rewardsAdsViewed: null,
        minEarningsPreviousMonth: 0,
      })
    }
  }

  async function updateTosUpdateRequired() {
    const { updateRequired } =
      await rewardsHandler.getTermsOfServiceUpdateRequired()
    store.update({ tosUpdateRequired: updateRequired })
  }

  async function loadData() {
    await Promise.all([
      updatePrefs(),
      updateRewardsEnabled(),
      updateExternalWallet(),
      updateParameters(),
      updateTosUpdateRequired(),
    ])

    store.update({ initialized: true })

    updateBalance()
    updateAdsData()
  }

  newTabProxy.addListeners({
    onRewardsStateUpdated: debounce(loadData, 10),
  })

  rewardsProxy.callbackRouter.onRewardsStateUpdated.addListener(loadData)

  loadData()

  return {
    setShowRewardsWidget(showRewardsWidget) {
      newTabHandler.setShowRewardsWidget(showRewardsWidget)
    },
  }
}
