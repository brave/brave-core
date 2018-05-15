/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/shieldsPanelTypes'
import { BlockTypes, BlockOptions, BlockFPOptions, BlockCookiesOptions } from '../other/blockTypes'

export interface ShieldDetails {
  id: number
  ads: BlockOptions
  trackers: BlockOptions
  httpUpgradableResources: BlockOptions
  javascript: BlockOptions
  fingerprinting: BlockFPOptions
  cookies: BlockCookiesOptions
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

export interface ShieldsPanelDataUpdated {
  (details: ShieldDetails): ShieldsPanelDataUpdatedReturn
}

interface ShieldsToggledReturn {
  type: types.SHIELDS_TOGGLED
  setting: BlockOptions
}

export interface ShieldsToggled {
  (setting: BlockOptions): ShieldsToggledReturn
}

interface ResourceBlockedReturn {
  type: types.RESOURCE_BLOCKED
  details: BlockDetails
}

export interface ResourceBlocked {
  (details: BlockDetails): ResourceBlockedReturn
}

interface BlockAdsTrackersReturn {
  type: types.BLOCK_ADS_TRACKERS
  setting: BlockOptions
}

export interface BlockAdsTrackers {
  (setting: BlockOptions): BlockAdsTrackersReturn
}

interface BlockFingerprintingReturn {
  type: types.BLOCK_FINGERPRINTING
  setting: BlockFPOptions
}

export interface BlockFingerprinting {
  (setting: BlockFPOptions): BlockFingerprintingReturn
}

interface BlockCookiesReturn {
  type: types.BLOCK_COOKIES
  setting: BlockCookiesOptions
}

export interface BlockCookies {
  (setting: BlockCookiesOptions): BlockCookiesReturn
}

interface ControlsToggledReturn {
  type: types.CONTROLS_TOGGLED
  setting: boolean
}

export interface ControlsToggled {
  (setting: boolean): ControlsToggledReturn
}

interface HttpsEverywhereToggledReturn {
  type: types.HTTPS_EVERYWHERE_TOGGLED
}

export interface HttpsEverywhereToggled {
  (): HttpsEverywhereToggledReturn
}

interface JavascriptToggledReturn {
  type: types.JAVASCRIPT_TOGGLED
}

export interface JavascriptToggled {
  (): JavascriptToggledReturn
}

interface AllowScriptOriginsOnceReturn {
  type: types.ALLOW_SCRIPT_ORIGINS_ONCE,
  origins: string[]
}

export interface AllowScriptOriginsOnce {
  (origins: string[]): AllowScriptOriginsOnceReturn
}

interface ChangeNoScriptSettingsReturn {
  type: types.CHANGE_NO_SCRIPT_SETTINGS,
  origin: string
}

export interface ChangeNoScriptSettings {
  (origin: string): ChangeNoScriptSettingsReturn
}

export type shieldPanelActions =
  ShieldsPanelDataUpdatedReturn |
  ShieldsToggledReturn |
  ResourceBlockedReturn |
  BlockAdsTrackersReturn |
  ControlsToggledReturn |
  HttpsEverywhereToggledReturn |
  JavascriptToggledReturn |
  BlockFingerprintingReturn |
  BlockCookiesReturn |
  AllowScriptOriginsOnceReturn |
  ChangeNoScriptSettingsReturn
