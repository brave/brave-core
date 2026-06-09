/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ClockFormat } from '../state/new_tab_store'
import {
  SelectedBackgroundType,
  solidPreviewBackground,
  type BackgroundStore,
} from '../state/background_store'
import { ConnectionState, type VpnStore } from '../state/vpn_store'
import type { NewTabStore } from '../state/new_tab_store'
import type { RewardsStore } from '../state/rewards_store'
import type { SearchStore } from '../state/search_store'
import type { TopSitesStore } from '../state/top_sites_store'

import { updateSponsoredBackground } from './mock_background_store'
import {
  setBraveNewsFeedV2FeatureFlagEnabled,
  setBraveNewsStoryConfiguration,
} from './mock_brave_news_setup'

export type BackgroundStoryType =
  | 'gradient'
  | 'solid'
  | 'brave'
  | 'custom'
  | 'disabled'

export type SponsoredBackgroundStoryType = 'none' | 'image' | 'rich'

export type VpnConnectionStoryState = 'connected' | 'connecting' | 'disconnected'

export type SettingsStoryView =
  | 'none'
  | 'background'
  | 'search'
  | 'top-sites'
  | 'clock'
  | 'widgets'
  | 'news'

export interface StorybookArgs {
  viewportWidth: number
  backgroundType: BackgroundStoryType
  sponsoredBackgroundType: SponsoredBackgroundStoryType
  centerNttCtaButton: boolean
  showClock: boolean
  clockFormat: 'auto' | '12' | '24'
  showTopSites: boolean
  showShieldsStats: boolean
  showTalkWidget: boolean
  showVpnWidget: boolean
  showRewardsWidget: boolean
  aiChatInputEnabled: boolean
  showSearchBox: boolean
  newsFeatureEnabled: boolean
  newsShowOnNTP: boolean
  newsOptedIn: boolean
  vpnPurchased: boolean
  vpnConnectionState: VpnConnectionStoryState
  rewardsEnabled: boolean
  openSettings: SettingsStoryView
}

export const defaultStorybookArgs: StorybookArgs = {
  viewportWidth: 1400,
  backgroundType: 'brave',
  sponsoredBackgroundType: 'none',
  centerNttCtaButton: false,
  showClock: true,
  clockFormat: 'auto',
  showTopSites: true,
  showShieldsStats: true,
  showTalkWidget: true,
  showVpnWidget: true,
  showRewardsWidget: true,
  aiChatInputEnabled: true,
  showSearchBox: true,
  newsFeatureEnabled: true,
  newsShowOnNTP: true,
  newsOptedIn: true,
  vpnPurchased: true,
  vpnConnectionState: 'connected',
  rewardsEnabled: true,
  openSettings: 'none',
}

interface StorybookStores {
  newTab: NewTabStore
  background: BackgroundStore
  search: SearchStore
  topSites: TopSitesStore
  vpn: VpnStore
  rewards: RewardsStore
}

const sampleBackground =
  'https://brave.com/static-assets/images/coding-background-texture.jpg'

function clockFormatFromStory(format: StorybookArgs['clockFormat']) {
  switch (format) {
    case '12':
      return ClockFormat.k12
    case '24':
      return ClockFormat.k24
    default:
      return ClockFormat.kAuto
  }
}

function vpnConnectionStateFromStory(state: VpnConnectionStoryState) {
  switch (state) {
    case 'connecting':
      return ConnectionState.CONNECTING
    case 'disconnected':
      return ConnectionState.DISCONNECTED
    default:
      return ConnectionState.CONNECTED
  }
}

export function applyStorybookArgs(stores: StorybookStores, args: StorybookArgs) {
  stores.newTab.update({
    showClock: args.showClock,
    clockFormat: clockFormatFromStory(args.clockFormat),
    showShieldsStats: args.showShieldsStats,
    showTalkWidget: args.showTalkWidget,
    talkFeatureEnabled: args.showTalkWidget,
    newsFeatureEnabled: args.newsFeatureEnabled,
    aiChatInputEnabled: args.aiChatInputEnabled,
    centerNttCtaButtonFeatureEnabled: args.centerNttCtaButton,
  })

  stores.background.update({
    backgroundsEnabled: args.backgroundType !== 'disabled',
    selectedBackground:
      args.backgroundType === 'solid'
        ? {
            type: SelectedBackgroundType.kSolid,
            value: solidPreviewBackground,
          }
        : args.backgroundType === 'custom'
          ? {
              type: SelectedBackgroundType.kCustom,
              value: sampleBackground,
            }
          : args.backgroundType === 'brave'
            ? {
                type: SelectedBackgroundType.kBrave,
                value: sampleBackground,
              }
            : {
                type: SelectedBackgroundType.kGradient,
                value: '',
              },
  })
  updateSponsoredBackground(stores.background, args.sponsoredBackgroundType)

  stores.search.update({
    showSearchBox: args.showSearchBox,
    showChatInput: args.aiChatInputEnabled,
  })

  stores.topSites.update({
    showTopSites: args.showTopSites,
  })

  stores.vpn.update({
    showVpnWidget: args.showVpnWidget,
    vpnFeatureEnabled: args.showVpnWidget,
    vpnPurchased: args.vpnPurchased,
    vpnConnectionState: vpnConnectionStateFromStory(args.vpnConnectionState),
  })

  stores.rewards.update({
    showRewardsWidget: args.showRewardsWidget,
    rewardsFeatureEnabled: args.showRewardsWidget,
    rewardsEnabled: args.rewardsEnabled,
    rewardsBalance: args.rewardsEnabled ? 1.204 : null,
  })

  setBraveNewsFeedV2FeatureFlagEnabled(args.newsFeatureEnabled)
  setBraveNewsStoryConfiguration({
    isOptedIn: args.newsOptedIn,
    showOnNTP: args.newsShowOnNTP,
  })
}

export function settingsUrlForStory(view: SettingsStoryView) {
  if (view === 'none') {
    return '/'
  }
  if (view === 'news') {
    return '/?openSettings=BraveNews'
  }
  return `/?openSettings=${view}`
}
