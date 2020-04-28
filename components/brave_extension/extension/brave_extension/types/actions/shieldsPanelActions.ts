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

interface ReportBrokenSiteReturn {
  type: types.REPORT_BROKEN_SITE
}

export interface ReportBrokenSite {
  (): ReportBrokenSiteReturn
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
  type: types.HTTPS_EVERYWHERE_TOGGLED,
  setting: BlockOptions
}

export interface HttpsEverywhereToggled {
  (setting: BlockOptions): HttpsEverywhereToggledReturn
}

interface BlockJavaScriptReturn {
  type: types.JAVASCRIPT_TOGGLED,
  setting: BlockJSOptions
}

export interface BlockJavaScript {
  (setting: BlockJSOptions): BlockJavaScriptReturn
}

interface AllowScriptOriginsOnceReturn {
  type: types.ALLOW_SCRIPT_ORIGINS_ONCE
}

export interface AllowScriptOriginsOnce {
  (): AllowScriptOriginsOnceReturn
}

interface SetScriptBlockedCurrentStateReturn {
  type: types.SET_SCRIPT_BLOCKED_ONCE_CURRENT_STATE,
  url: string
}

export interface SetScriptBlockedCurrentState {
  (url: string): SetScriptBlockedCurrentStateReturn
}

interface SetGroupedScriptsBlockedCurrentStateReturn {
  type: types.SET_GROUPED_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE,
  origin: string,
  maybeBlock: boolean
}

export interface SetGroupedScriptsBlockedCurrentState {
  (origin: string, maybeBlock: boolean): SetGroupedScriptsBlockedCurrentStateReturn
}

interface SetAllScriptsBlockedCurrentStateReturn {
  type: types.SET_ALL_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE,
  maybeBlock: boolean
}

export interface SetAllScriptsBlockedCurrentState {
  (maybeBlock: boolean): SetAllScriptsBlockedCurrentStateReturn
}

interface SetFinalScriptsBlockedStateReturn {
  type: types.SET_FINAL_SCRIPTS_BLOCKED_ONCE_STATE
}

export interface SetFinalScriptsBlockedState {
  (): SetFinalScriptsBlockedStateReturn
}

interface SetAdvancedViewFirstAccessReturn {
  type: types.SET_ADVANCED_VIEW_FIRST_ACCESS
}

export interface SetAdvancedViewFirstAccess {
  (): SetAdvancedViewFirstAccessReturn
}

interface ShieldsReadyReturn {
  type: types.SHIELDS_READY
}

export interface ShieldsReady {
  (): ShieldsReadyReturn
}

interface GenerateClassIdStylesheetReturn {
  type: types.GENERATE_CLASS_ID_STYLESHEET,
  tabId: number,
  classes: string[],
  ids: string[]
}

export interface GenerateClassIdStylesheet {
  (tabId: number, classes: string[], ids: string[]): GenerateClassIdStylesheetReturn
}

interface CosmeticFilterRuleExceptionsReturn {
  type: types.COSMETIC_FILTER_RULE_EXCEPTIONS,
  tabId: number,
  exceptions: string[],
  scriptlet: string
}

export interface CosmeticFilterRuleExceptions {
  (tabId: number, exceptions: string[], scriptlet: string): CosmeticFilterRuleExceptionsReturn
}

interface ContentScriptsLoadedReturn {
  type: types.CONTENT_SCRIPTS_LOADED,
  tabId: number,
  url: string,
}

export interface ContentScriptsLoaded {
  (tabId: number, url: string): ContentScriptsLoadedReturn
}

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
  ShieldsReadyReturn |
  GenerateClassIdStylesheetReturn |
  CosmeticFilterRuleExceptionsReturn |
  ContentScriptsLoadedReturn
