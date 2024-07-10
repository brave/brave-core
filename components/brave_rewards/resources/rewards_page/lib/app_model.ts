/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

export interface AppState {
  loading: boolean
  openTime: number
  embedder: EmbedderInfo
  paymentId: string
}

export type AppStateListener = (state: AppState) => void

export interface AppModel {
  getState: () => AppState
  addListener: (callback: AppStateListener) => () => void
  onAppRendered: () => void
  openTab: (url: string) => void
  enableRewards: (countryCode: string) => Promise<EnableRewardsResult>
  getAvailableCountries: () => Promise<AvailableCountryInfo>
  resetRewards: () => Promise<void>
}

export function defaultState (): AppState {
  return {
    loading: true,
    openTime: Date.now(),
    embedder: {
      isBubble: false,
      platform: 'desktop',
      animatedBackgroundEnabled: false
    },
    paymentId: ''
  }
}

export function defaultModel (): AppModel {
  const state = defaultState()
  return {
    getState() { return state },

    addListener() { return () => {} },

    onAppRendered() {},

    openTab() {},

    async enableRewards(countryCode) {
      return 'unexpected-error'
    },

    async getAvailableCountries() {
      return {
        countryCodes: ['US'],
        defaultCountryCode: 'US'
      }
    },

    async resetRewards() {}
  }
}
