/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { UserType } from '../../shared/lib/user_type'

import * as mojom from '../../shared/lib/mojom'
import { PublisherStatus } from '../../shared/lib/publisher_status'

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

export interface ExternalWalletInfo {
  status: mojom.WalletStatus
  type: 'uphold' | 'bitflyer' | 'gemini'
}

export interface RewardsParameters {
  rate: number
  tipChoices: number[]
  monthlyTipChoices: number[]
  vbatExpired: boolean
}

export interface HostError {
  type: string
  code?: number
}

export interface HostState {
  publisherInfo?: PublisherInfo
  balanceInfo?: mojom.Balance
  externalWalletInfo?: ExternalWalletInfo
  userType?: UserType
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
