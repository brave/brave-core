/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { loadTimeData } from '../i18n_setup.js';

export interface BraveRewardsBrowserProxy {
  getAdsData(): Promise<any>
  getRewardsEnabled(): Promise<boolean>
  getRewardsParameters(): Promise<any>
  getUserType(): Promise<boolean>
  isAutoContributeSupported(): Promise<boolean>
  wasInlineTipButtonsEnabledAtStartup(): boolean
  wasInlineTipTwitterEnabledAtStartup(): boolean
  wasInlineTipRedditEnabledAtStartup(): boolean
  wasInlineTipGithubEnabledAtStartup(): boolean
}

/**
 * @implements {settings.BraveRewardsBrowserProxy}
 */
export class BraveRewardsBrowserProxyImpl implements BraveRewardsBrowserProxy {
  getAdsData () {
    return new Promise((resolve) => chrome.braveRewards.getAdsData(
      (data) => { resolve(data) }))
  }

  getRewardsEnabled () {
    return new Promise<boolean>((resolve) => chrome.braveRewards.getRewardsEnabled(
      (enabled) => { resolve(enabled) }))
  }

  getRewardsParameters () {
    return new Promise((resolve) => chrome.braveRewards.getRewardsParameters(
      (parameters) => { resolve(parameters) }))
  }

  getUserType () {
    return new Promise((resolve) => chrome.braveRewards.getUserType(
      (userType) => { resolve(userType) }))
  }

  isAutoContributeSupported () {
    return new Promise<boolean>((resolve) => chrome.braveRewards.isAutoContributeSupported(
      (supported) => { resolve(supported) }))
  }

  wasInlineTipButtonsEnabledAtStartup() {
    return loadTimeData.getBoolean('inlineTipButtonsEnabled')
  }

  wasInlineTipTwitterEnabledAtStartup() {
    return loadTimeData.getBoolean('inlineTipTwitterEnabled')
  }

  wasInlineTipRedditEnabledAtStartup() {
    return loadTimeData.getBoolean('inlineTipRedditEnabled')
  }

  wasInlineTipGithubEnabledAtStartup() {
    return loadTimeData.getBoolean('inlineTipGithubEnabled')
  }

  static getInstance() {
    return instance || (instance = new BraveRewardsBrowserProxyImpl())
  }
}

let instance: BraveRewardsBrowserProxy|null = null
