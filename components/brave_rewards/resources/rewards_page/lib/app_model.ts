/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet, ExternalWalletProvider } from '../../shared/lib/external_wallet'
import { Optional } from '../../shared/lib/optional'

interface EmbedderInfo {
  isBubble: boolean
  platform: 'android' | 'desktop'
  animatedBackgroundEnabled: boolean
}

export type EnableRewardsResult =
  'success' |
  'wallet-generation-disabled' |
  'country-already-declared' |
  'unexpected-error'

export interface AvailableCountryInfo {
  countryCodes: string[]
  defaultCountryCode: string
}

export type AdType =
  'new-tab-page' |
  'notification' |
  'search-result' |
  'inline-content'

export interface AdsInfo {
  adsEnabled: Record<AdType, boolean>
  adsReceivedThisMonth: number
  adTypesReceivedThisMonth: Record<AdType, number>
  minEarningsThisMonth: number
  maxEarningsThisMonth: number
  nextPaymentDate: number
  notificationAdsPerHour: number
  shouldAllowSubdivisionTargeting: boolean
  currentSubdivision: string
  availableSubdivisions: Array<{ code: string, name: string }>
  autoDetectedSubdivision: string
}

export type AdLikeStatus = 'liked' | 'disliked' | ''

export interface AdsHistoryItem {
  id: string
  createdAt: number
  name: string
  text: string
  domain: string
  url: string
  likeStatus: AdLikeStatus
  inappropriate: boolean
}

export interface RewardsParameters {
  walletProviderRegions: Record<string, { allow: string[], block: string[] }>
}

export type ConnectExternalWalletResult =
  'success' |
  'device-limit-reached' |
  'flagged-wallet' |
  'kyc-required' |
  'mismatched-countries' |
  'mismatched-provider-accounts' |
  'provider-unavailable' |
  'region-not-supported' |
  'request-signature-verification-error' |
  'unexpected-error' |
  'uphold-bat-not-allowed' |
  'uphold-insufficient-capabilities' |
  'uphold-transaction-verification-failure'

export interface AppState {
  loading: boolean
  openTime: number
  embedder: EmbedderInfo
  paymentId: string
  countryCode: string
  externalWallet: ExternalWallet | null
  balance: Optional<number>
  adsInfo: AdsInfo | null
  rewardsParameters: RewardsParameters | null
}

export type AppStateListener = (state: AppState) => void

export interface AppModel {
  getState: () => AppState
  addListener: (callback: AppStateListener) => () => void
  onAppRendered: () => void
  openTab: (url: string) => void
  getString: (key: string) => string
  getPluralString: (key: string, count: number) => Promise<string>
  enableRewards: (countryCode: string) => Promise<EnableRewardsResult>
  getAvailableCountries: () => Promise<AvailableCountryInfo>
  getExternalWalletProviders: () => Promise<ExternalWalletProvider[]>
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
}

export function defaultState(): AppState {
  return {
    loading: true,
    openTime: Date.now(),
    embedder: {
      isBubble: false,
      platform: 'desktop',
      animatedBackgroundEnabled: false
    },
    paymentId: '',
    countryCode: '',
    externalWallet: null,
    balance: new Optional(),
    adsInfo: null,
    rewardsParameters: null
  }
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

    async getAvailableCountries() {
      return {
        countryCodes: [],
        defaultCountryCode: ''
      }
    },

    async getExternalWalletProviders() { return [] },

    async beginExternalWalletLogin(provider) { return true },

    async connectExternalWallet(provider, args) { return 'unexpected-error' },

    async resetRewards() {},

    async setAdTypeEnabled(adType, enabled) {},

    async setNotificationAdsPerHour(adsPerHour) {},

    async setAdsSubdivision(subdivision) {},

    async getAdsHistory() { return [] },

    async setAdLikeStatus(id, status) {},

    async setAdInappropriate(id, value) {}
  }
}
