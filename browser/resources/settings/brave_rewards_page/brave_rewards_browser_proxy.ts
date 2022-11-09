/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

export interface BraveRewardsBrowserProxy {
  getAdsData(): Promise<any>
  getRewardsEnabled(): Promise<boolean>
  getRewardsParameters(): Promise<any>
  isAutoContributeSupported(): Promise<boolean>
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

  isAutoContributeSupported () {
    return new Promise<boolean>((resolve) => chrome.braveRewards.isAutoContributeSupported(
      (supported) => { resolve(supported) }))
  }

  static getInstance() {
    return instance || (instance = new BraveRewardsBrowserProxyImpl())
  }
}

let instance: BraveRewardsBrowserProxy|null = null
