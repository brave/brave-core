/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import { RewardsPageProxy } from '../../../../components/brave_rewards/resources/rewards_page/webui/rewards_page_proxy'
import { externalWalletFromExtensionData } from '../../../../components/brave_rewards/resources/shared/lib/external_wallet'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { createStore } from '../lib/store'
import { debounceListener } from './debounce_listener'

import {
  RewardsModel,
  defaultState,
  defaultModel } from '../models/rewards_model'

export function createRewardsModel(): RewardsModel {
  if (!loadTimeData.getBoolean('rewardsFeatureEnabled')) {
    return defaultModel()
  }

  const newTabProxy = NewTabPageProxy.getInstance()
  const newTabHandler = newTabProxy.handler
  const rewardsProxy = RewardsPageProxy.getInstance()
  const rewardsHandler = rewardsProxy.handler
  const store = createStore({ ...defaultState(), rewardsFeatureEnabled: true })

  async function updatePrefs() {
    const { showRewardsWidget } = await newTabHandler.getShowRewardsWidget()
    store.update({ showRewardsWidget })
  }

  async function updateRewardsEnabled() {
    const { paymentId } = await rewardsHandler.getRewardsPaymentId()
    store.update({ rewardsEnabled: Boolean(paymentId) })
  }

  async function updateExternalWallet() {
    const { externalWallet } = await rewardsHandler.getExternalWallet()
    store.update({
      externalWallet: externalWalletFromExtensionData(externalWallet)
    })
  }

  async function loadData() {
    await Promise.all([
      updatePrefs(),
      updateRewardsEnabled(),
      updateExternalWallet()
    ])
  }

  newTabProxy.addListeners({
    onRewardsStateUpdated: debounceListener(updatePrefs)
  })

  rewardsProxy.callbackRouter.onRewardsStateUpdated.addListener(loadData)

  loadData()

  return {
    getState: store.getState,

    addListener: store.addListener,

    setShowRewardsWidget(showRewardsWidget) {
      newTabHandler.setShowRewardsWidget(showRewardsWidget)
    }
  }
}
