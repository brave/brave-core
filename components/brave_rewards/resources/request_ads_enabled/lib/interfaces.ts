/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export interface ClientState {
  loading: boolean
  showRewardsOnboarding: boolean
}

export type ClientListener = (state: ClientState) => void

export interface Client {
  state: ClientState
  addListener: (callback: ClientListener) => (() => void)
  getString: (key: string) => string
  openRewardsTour: () => void
  enableAds: () => void
}
