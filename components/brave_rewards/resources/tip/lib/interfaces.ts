/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from '../../shared/lib/mojom'

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

export enum PublisherStatus {
  NOT_VERIFIED = 0,
  CONNECTED = 1,
  UPHOLD_VERIFIED = 2,
  BITFLYER_VERIFIED = 3,
  GEMINI_VERIFIED = 4
}

export interface PublisherInfo {
  publisherKey: string
  name: string
  title: string
  description: string
  background: string
  logo: string
  provider: string
  links: Record<string, string>
  status: PublisherStatus
}

export interface BalanceInfo {
  total: number
  wallets: Record<string, number>
}

export interface ExternalWalletInfo {
  status: mojom.WalletStatus
  type: 'uphold' | 'bitflyer' | 'gemini'
}

export interface RewardsParameters {
  rate: number
  tipChoices: number[]
  monthlyTipChoices: number[]
}

export interface HostError {
  type: string
  code?: number
}

export interface HostState {
  publisherInfo?: PublisherInfo
  balanceInfo?: BalanceInfo
  externalWalletInfo?: ExternalWalletInfo
  userVersion?: string
  rewardsParameters?: RewardsParameters
  hostError?: HostError
  nextReconcileDate?: Date
  currentMonthlyTip?: number
  tipProcessed?: boolean
  tipAmount?: number
  tipPending?: boolean
}

export type HostListener = (state: HostState) => void

export interface Host {
  state: HostState
  getDialogArgs: () => DialogArgs
  closeDialog: () => void
  processTip: (amount: number, kind: TipKind) => void
  shareTip: (target: ShareTarget) => void
  addListener: (callback: HostListener) => () => void
}
