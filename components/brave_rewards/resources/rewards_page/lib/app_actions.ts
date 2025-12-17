/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  AdsHistoryItem,
  AdLikeStatus,
  AdType,
  EnableRewardsResult,
  ExternalWalletProvider,
  AvailableCountryInfo,
  ConnectExternalWalletResult,
} from './app_state'

export interface AppActions {
  onAppRendered: () => void
  openTab: (url: string) => void
  getPluralString: (key: string, count: number) => Promise<string>
  enableRewards: (countryCode: string) => Promise<EnableRewardsResult>
  setWebDiscoveryProjectEnabled: (enabled: boolean) => Promise<void>
  getAvailableCountries: () => Promise<AvailableCountryInfo>
  beginExternalWalletLogin: (
    provider: ExternalWalletProvider,
  ) => Promise<boolean>
  connectExternalWallet: (
    provider: ExternalWalletProvider,
    args: Record<string, string>,
  ) => Promise<ConnectExternalWalletResult>
  resetRewards: () => Promise<void>
  setAdTypeEnabled: (adType: AdType, enabled: boolean) => Promise<void>
  setNotificationAdsPerHour: (adsPerHour: number) => Promise<void>
  setAdsSubdivision: (subdivision: string) => Promise<void>
  getAdsHistory: () => Promise<AdsHistoryItem[]>
  setAdLikeStatus: (id: string, status: AdLikeStatus) => Promise<void>
  setAdInappropriate: (id: string, value: boolean) => Promise<void>
  removeRecurringContribution: (id: string) => Promise<void>
  sendContribution: (
    creatorID: string,
    amount: number,
    recurring: boolean,
  ) => Promise<boolean>
  acceptTermsOfServiceUpdate: () => Promise<void>
  dismissSelfCustodyInvite: () => Promise<void>
  onCaptchaResult: (success: boolean) => Promise<void>
  clearNotification: (id: string) => Promise<void>
  recordOfferClick: () => Promise<void>
  recordOfferView: () => Promise<void>
}

export function defaultActions(): AppActions {
  return {
    onAppRendered() {},
    openTab() {},
    async getPluralString() {
      return ''
    },
    async enableRewards() {
      return 'unexpected-error'
    },
    async setWebDiscoveryProjectEnabled() {},
    async getAvailableCountries() {
      return { countryCodes: [], defaultCountryCode: '' }
    },
    async beginExternalWalletLogin() {
      return true
    },
    async connectExternalWallet() {
      return 'unexpected-error'
    },
    async resetRewards() {},
    async setAdTypeEnabled() {},
    async setNotificationAdsPerHour() {},
    async setAdsSubdivision() {},
    async getAdsHistory() {
      return []
    },
    async setAdLikeStatus() {},
    async setAdInappropriate() {},
    async removeRecurringContribution() {},
    async sendContribution() {
      return false
    },
    async acceptTermsOfServiceUpdate() {},
    async dismissSelfCustodyInvite() {},
    async onCaptchaResult() {},
    async clearNotification() {},
    async recordOfferClick() {},
    async recordOfferView() {},
  }
}
