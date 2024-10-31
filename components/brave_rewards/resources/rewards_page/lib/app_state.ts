/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet, ExternalWalletProvider } from '../../shared/lib/external_wallet'
import { Notification } from '../../shared/components/notifications'
import { ProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
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
  browserUpgradeRequired: boolean
  isSupportedRegion: boolean
  adsEnabled: Record<AdType, boolean>
  adsReceivedThisMonth: number
  adTypesReceivedThisMonth: Record<AdType, number>
  minEarningsThisMonth: number
  maxEarningsThisMonth: number
  minEarningsPreviousMonth: number
  maxEarningsPreviousMonth: number
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
  autoContributeChoices: number[]
  autoContributeChoice: number
  tipChoices: number[]
  rate: number
  walletProviderRegions: Record<string, { allow: string[], block: string[] }>
  payoutStatus: Record<string, ProviderPayoutStatus>
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

export type CreatorPlatform =
  '' |
  'twitter' |
  'youtube' |
  'twitch' |
  'reddit' |
  'vimeo' |
  'github'

export interface CreatorSite {
  id: string
  icon: string
  name: string
  url: string
  platform: CreatorPlatform
}

export interface CreatorBanner {
  title: string
  description: string
  background: string
  web3URL: string
}

export interface CreatorInfo {
  site: CreatorSite
  banner: CreatorBanner
  supportedWalletProviders: ExternalWalletProvider[]
}

export interface AutoContributeEntry {
  site: CreatorSite
  attention: number
}

export interface AutoContributeInfo {
  enabled: boolean
  amount: number
  nextAutoContributeDate: number
  entries: AutoContributeEntry[]
}

export interface RecurringContribution {
  site: CreatorSite
  amount: number
  nextContributionDate: number
}

export interface CaptchaInfo {
  url: string
  maxAttemptsExceeded: boolean
}

export { Notification }

export { ExternalWallet, ExternalWalletProvider }

export interface UICardItem {
  title: string
  description: string
  url: string
  thumbnail: string
}

export interface UICard {
  name: string
  items: UICardItem[]
}

export interface AppState {
  loading: boolean
  openTime: number
  embedder: EmbedderInfo
  paymentId: string
  countryCode: string
  externalWallet: ExternalWallet | null
  externalWalletProviders: ExternalWalletProvider[]
  balance: Optional<number>
  tosUpdateRequired: boolean
  selfCustodyInviteDismissed: boolean
  adsInfo: AdsInfo | null
  autoContributeInfo: AutoContributeInfo | null
  recurringContributions: RecurringContribution[]
  rewardsParameters: RewardsParameters | null
  currentCreator: CreatorInfo | null
  captchaInfo: CaptchaInfo | null
  notifications: Notification[]
  cards: UICard[] | null
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
    externalWalletProviders: [],
    balance: new Optional(),
    tosUpdateRequired: false,
    selfCustodyInviteDismissed: false,
    adsInfo: null,
    autoContributeInfo: null,
    recurringContributions: [],
    rewardsParameters: null,
    currentCreator: null,
    captchaInfo: null,
    notifications: [],
    cards: null
  }
}
