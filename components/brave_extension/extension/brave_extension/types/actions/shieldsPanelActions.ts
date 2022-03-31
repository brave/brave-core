/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/shieldsPanelTypes'
import { BlockTypes, BlockOptions, BlockFPOptions, BlockJSOptions, BlockCookiesOptions } from '../other/blockTypes'

export interface ShieldDetails {
  id: number
  cosmeticBlocking: boolean
  ads: BlockOptions
  trackers: BlockOptions
  firstPartyCosmeticFiltering: boolean
  httpUpgradableResources: BlockOptions
  javascript: BlockOptions
  fingerprinting: BlockFPOptions
  cookies: BlockCookiesOptions
  url: string
  braveShields: boolean
  origin: string
  hostname: string
}

export interface BlockDetails {
  blockType: BlockTypes
  tabId: number
  subresource: string
}

interface ShieldsPanelDataUpdatedReturn {
  type: types.SHIELDS_PANEL_DATA_UPDATED
  details: ShieldDetails
}

export type ShieldsPanelDataUpdated = (details: ShieldDetails) => ShieldsPanelDataUpdatedReturn

interface ShieldsToggledReturn {
  type: types.SHIELDS_TOGGLED
  setting: BlockOptions
}

export type ShieldsToggled = (setting: BlockOptions) => ShieldsToggledReturn

interface ReportBrokenSiteReturn {
  type: types.REPORT_BROKEN_SITE
}

export type ReportBrokenSite = () => ReportBrokenSiteReturn

interface ResourceBlockedReturn {
  type: types.RESOURCE_BLOCKED
  details: BlockDetails
}

export type ResourceBlocked = (details: BlockDetails) => ResourceBlockedReturn

interface BlockAdsTrackersReturn {
  type: types.BLOCK_ADS_TRACKERS
  setting: BlockOptions
}

export type BlockAdsTrackers = (setting: BlockOptions) => BlockAdsTrackersReturn

interface BlockFingerprintingReturn {
  type: types.BLOCK_FINGERPRINTING
  setting: BlockFPOptions
}

export type BlockFingerprinting = (setting: BlockFPOptions) => BlockFingerprintingReturn

interface BlockCookiesReturn {
  type: types.BLOCK_COOKIES
  setting: BlockCookiesOptions
}

export type BlockCookies = (setting: BlockCookiesOptions) => BlockCookiesReturn

interface ControlsToggledReturn {
  type: types.CONTROLS_TOGGLED
  setting: boolean
}

export type ControlsToggled = (setting: boolean) => ControlsToggledReturn

interface HttpsEverywhereToggledReturn {
  type: types.HTTPS_EVERYWHERE_TOGGLED
  setting: BlockOptions
}

export type HttpsEverywhereToggled = (setting: BlockOptions) => HttpsEverywhereToggledReturn

interface BlockJavaScriptReturn {
  type: types.JAVASCRIPT_TOGGLED
  setting: BlockJSOptions
}

export type BlockJavaScript = (setting: BlockJSOptions) => BlockJavaScriptReturn

interface AllowScriptOriginsOnceReturn {
  type: types.ALLOW_SCRIPT_ORIGINS_ONCE
}

export type AllowScriptOriginsOnce = () => AllowScriptOriginsOnceReturn

interface SetScriptBlockedCurrentStateReturn {
  type: types.SET_SCRIPT_BLOCKED_ONCE_CURRENT_STATE
  url: string
}

export type SetScriptBlockedCurrentState = (url: string) => SetScriptBlockedCurrentStateReturn

interface SetGroupedScriptsBlockedCurrentStateReturn {
  type: types.SET_GROUPED_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE
  origin: string
  maybeBlock: boolean
}

export type SetGroupedScriptsBlockedCurrentState = (origin: string, maybeBlock: boolean) => SetGroupedScriptsBlockedCurrentStateReturn

interface SetAllScriptsBlockedCurrentStateReturn {
  type: types.SET_ALL_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE
  maybeBlock: boolean
}

export type SetAllScriptsBlockedCurrentState = (maybeBlock: boolean) => SetAllScriptsBlockedCurrentStateReturn

interface SetFinalScriptsBlockedStateReturn {
  type: types.SET_FINAL_SCRIPTS_BLOCKED_ONCE_STATE
}

export type SetFinalScriptsBlockedState = () => SetFinalScriptsBlockedStateReturn

interface SetAdvancedViewFirstAccessReturn {
  type: types.SET_ADVANCED_VIEW_FIRST_ACCESS
}

export type SetAdvancedViewFirstAccess = () => SetAdvancedViewFirstAccessReturn

interface ShieldsReadyReturn {
  type: types.SHIELDS_READY
}

export type ShieldsReady = () => ShieldsReadyReturn

export type shieldPanelActions =
  ShieldsPanelDataUpdatedReturn |
  ShieldsToggledReturn |
  ReportBrokenSiteReturn |
  ResourceBlockedReturn |
  BlockAdsTrackersReturn |
  ControlsToggledReturn |
  HttpsEverywhereToggledReturn |
  BlockJavaScriptReturn |
  BlockFingerprintingReturn |
  BlockCookiesReturn |
  AllowScriptOriginsOnceReturn |
  SetScriptBlockedCurrentStateReturn |
  SetGroupedScriptsBlockedCurrentStateReturn |
  SetAllScriptsBlockedCurrentStateReturn |
  SetFinalScriptsBlockedStateReturn |
  SetAdvancedViewFirstAccessReturn |
  ShieldsReadyReturn
