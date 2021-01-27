/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

type EmptyMediaData = {
  mediaType: 'none'
}

type TwitterMediaData = {
  mediaType: 'twitter'
  publisherName: string
  postId: string
  postTimestamp: string
  postText: string
}

type RedditMediaData = {
  mediaType: 'reddit'
  publisherName: string
  postTimestamp: string
  postText: string
}

type GithubMediaData = {
  mediaType: 'github'
  publisherName: string
  publisherScreenName: string
}

export type MediaMetaData =
  EmptyMediaData |
  TwitterMediaData |
  RedditMediaData |
  GithubMediaData

export type ShareTarget = 'twitter'

export type EntryPoint =
  'one-time' |
  'set-monthly' |
  'clear-monthly'

export interface DialogArgs {
  url: string
  entryPoint: EntryPoint
  publisherKey: string
  mediaMetaData: MediaMetaData
}

export type TipKind = 'one-time' | 'monthly'

export type PaymentKind = 'bat'

export type OnboardingResult = 'opted-in' | 'dismissed'

export enum PublisherStatus {
  NOT_VERIFIED = 0,
  CONNECTED = 1,
  UPHOLD_VERIFIED = 2,
  BITFLYER_VERIFIED = 3
}

export interface PublisherInfo {
  publisherKey: string
  name: string
  title: string
  description: string
  background: string
  logo: string
  amounts: number[]
  provider: string
  links: Record<string, string>
  status: PublisherStatus
}

export interface BalanceInfo {
  total: number
  wallets: Record<string, number>
}

export enum ExternalWalletStatus {
  NOT_CONNECTED = 0,
  CONNECTED = 1,
  VERIFIED = 2,
  DISCONNECTED_NOT_VERIFIED = 3,
  DISCONNECTED_VERIFIED = 4
}

export interface ExternalWalletInfo {
  token: string
  address: string
  status: ExternalWalletStatus
  type: 'anonymous' | 'uphold' | 'bitflyer'
  verifyUrl: string
  addUrl: string
  withdrawUrl: string
  userName: string
  accountUrl: string
  loginUrl: string
}

export interface RewardsParameters {
  rate: number
  tipChoices: number[]
  monthlyTipChoices: number[]
  autoContributeChoices: number[]
}

export interface HostError {
  type: string
  code?: number
}

export interface HostState {
  publisherInfo?: PublisherInfo
  balanceInfo?: BalanceInfo
  externalWalletInfo?: ExternalWalletInfo
  rewardsParameters?: RewardsParameters
  hostError?: HostError
  nextReconcileDate?: Date
  adsPerHour?: number
  autoContributeAmount?: number
  currentMonthlyTip?: number
  onlyAnonWallet?: boolean
  showOnboarding?: boolean
  tipProcessed?: boolean
  tipAmount?: number
}

export type HostListener = (state: HostState) => void

export interface Host {
  state: HostState
  getString: (key: string) => string
  getDialogArgs: () => DialogArgs
  closeDialog: () => void
  setAutoContributeAmount: (autoContributeAmount: number) => void
  setAdsPerHour: (adsPerHour: number) => void
  saveOnboardingResult: (result: OnboardingResult) => void
  processTip: (amount: number, kind: TipKind) => void
  shareTip: (target: ShareTarget) => void
  addListener: (callback: HostListener) => () => void
}
