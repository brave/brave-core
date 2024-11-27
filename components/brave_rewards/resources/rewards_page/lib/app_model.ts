/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  AppState,
  AdsHistoryItem,
  AdLikeStatus,
  AdType,
  EnableRewardsResult,
  ExternalWalletProvider,
  AvailableCountryInfo,
  ConnectExternalWalletResult,
  defaultState } from './app_state'

export type AppStateListener = (state: AppState) => void

export interface AppModel {
  getState: () => AppState
  addListener: (callback: AppStateListener) => () => void
  onAppRendered: () => void
  openTab: (url: string) => void
  getString: (key: string) => string
  getPluralString: (key: string, count: number) => Promise<string>
  enableRewards: (countryCode: string) => Promise<EnableRewardsResult>
  setWebDiscoveryProjectEnabled: (enabled: boolean) => Promise<void>
  getAvailableCountries: () => Promise<AvailableCountryInfo>
  beginExternalWalletLogin:
    (provider: ExternalWalletProvider) => Promise<boolean>
  connectExternalWallet:
    (provider: ExternalWalletProvider, args: Record<string, string>)
      => Promise<ConnectExternalWalletResult>
  resetRewards: () => Promise<void>
  setAdTypeEnabled: (adType: AdType, enabled: boolean) => Promise<void>
  setNotificationAdsPerHour: (adsPerHour: number) => Promise<void>
  setAdsSubdivision: (subdivision: string) => Promise<void>
  getAdsHistory: () => Promise<AdsHistoryItem[]>
  setAdLikeStatus: (id: string, status: AdLikeStatus) => Promise<void>
  setAdInappropriate: (id: string, value: boolean) => Promise<void>
  removeRecurringContribution: (id: string) => Promise<void>
  sendContribution:
    (creatorID: string, amount: number, recurring: boolean) => Promise<boolean>
  acceptTermsOfServiceUpdate: () => Promise<void>
  dismissSelfCustodyInvite: () => Promise<void>
  onCaptchaResult: (success: boolean) => Promise<void>
  clearNotification: (id: string) => Promise<void>
}

export function defaultModel(): AppModel {
  const state = defaultState()
  return {
    getState() { return state },

    addListener() { return () => {} },

    onAppRendered() {},

    openTab() {},

    getString(key) { return '' },

    async getPluralString(key, count) { return '' },

    async enableRewards(countryCode) { return 'unexpected-error' },

    async setWebDiscoveryProjectEnabled(enabled) {},

    async getAvailableCountries() {
      return {
        countryCodes: [],
        defaultCountryCode: ''
      }
    },

    async beginExternalWalletLogin(provider) { return true },

    async connectExternalWallet(provider, args) { return 'unexpected-error' },

    async resetRewards() {},

    async setAdTypeEnabled(adType, enabled) {},

    async setNotificationAdsPerHour(adsPerHour) {},

    async setAdsSubdivision(subdivision) {},

    async getAdsHistory() { return [] },

    async setAdLikeStatus(id, status) {},

    async setAdInappropriate(id, value) {},

    async removeRecurringContribution(id) {},

    async sendContribution(creatorID, amount, recurring) {
      return false
    },

    async acceptTermsOfServiceUpdate() {},

    async dismissSelfCustodyInvite() {},

    async onCaptchaResult(success) {},

    async clearNotification(id: string) {}
  }
}
