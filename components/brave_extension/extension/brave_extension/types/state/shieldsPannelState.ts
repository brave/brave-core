/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { BlockOptions, BlockTypes, BlockFPOptions, BlockCookiesOptions } from '../other/blockTypes'
import { NoScriptInfo } from '../other/noScriptInfo'
import { SettingsData } from '../other/settingsTypes'

export interface Tab {
  firstPartyCosmeticFiltering: boolean
  ads: BlockOptions
  adsBlocked: number
  controlsOpen: boolean
  hostname: string
  httpUpgradableResources: BlockOptions
  httpsRedirected: number
  id: number
  javascript: BlockOptions
  javascriptBlocked: number
  origin: string
  braveShields: BlockOptions
  trackers: BlockOptions
  trackersBlocked: number
  url: string
  fingerprinting: BlockFPOptions
  fingerprintingBlocked: number
  cookies: BlockCookiesOptions
  noScriptInfo: NoScriptInfo
  adsBlockedResources: Array<string>
  trackersBlockedResources: Array<string>
  httpsRedirectedResources: Array<string>
  fingerprintingBlockedResources: Array<string>
}

export interface Tabs {
  [key: number]: Tab
}

export interface Windows {
  [key: number]: number
}

export interface PersistentData {
  isFirstAccess: boolean
}

export interface State {
  persistentData: PersistentData
  settingsData: SettingsData
  currentWindowId: number
  tabs: Tabs
  windows: Windows
}

export interface GetActiveTabId {
  (state: State): number
}

export interface GetActiveTabData {
  (state: State): Tab | undefined
}

export interface GetPersistentData {
  (state: State): PersistentData
}

export interface UpdatePersistentData {
  (state: State, persistentData: Partial<PersistentData>): State
}

export interface MergeSettingsData {
  (state: State, settingsData: SettingsData): SettingsData
}

export interface UpdateActiveTab {
  (state: State, windowId: number, tabId: number): State
}

export interface RemoveWindowInfo {
  (state: State, windowId: number): State
}

export interface UpdateFocusedWindow {
  (state: State, windowId: number): State
}

export interface UpdateTabShieldsData {
  (state: State, tabId: number, details: Partial<Tab>): State
}

export interface UpdateResourceBlocked {
  (state: State, tabId: number, blockType: BlockTypes, subresource: string): State
}

export interface SaveCosmeticFilterRuleExceptions {
  (state: State, tabId: number, exceptions: Array<string>): State
}

export interface ResetBlockingStats {
  (state: State, tabId: number): State
}

export interface ResetBlockingResources {
  (state: State, tabId: number): State
}

export interface ResetNoScriptInfo {
  (state: State, tabId: number, newOrigin: string): State
}

export interface UpdateShieldsIconBadgeText {
  (state: State): void
}
export interface UpdateShieldsIconImage {
  (state: State): void
}
export interface UpdateShieldsIcon {
  (state: State): void
}
export interface FocusedWindowChanged {
  (state: State, windowId: number): State
}
export interface RequestDataAndUpdateActiveTab {
  (state: State, windowId: number, tabId: number): State
}

export interface PersistAllNoScriptSettings {
  (state: State, tabId: number): State
}
