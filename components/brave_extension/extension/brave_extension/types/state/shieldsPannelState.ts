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
  adsBlockedResources: string[]
  trackersBlockedResources: string[]
  httpsRedirectedResources: string[]
  fingerprintingBlockedResources: string[]
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

export type GetActiveTabId = (state: State) => number

export type GetActiveTabData = (state: State) => Tab | undefined

export type GetPersistentData = (state: State) => PersistentData

export type UpdatePersistentData = (state: State, persistentData: Partial<PersistentData>) => State

export type MergeSettingsData = (state: State, settingsData: SettingsData) => SettingsData

export type UpdateActiveTab = (state: State, windowId: number, tabId: number) => State

export type RemoveWindowInfo = (state: State, windowId: number) => State

export type UpdateFocusedWindow = (state: State, windowId: number) => State

export type UpdateTabShieldsData = (state: State, tabId: number, details: Partial<Tab>) => State

export type UpdateResourceBlocked = (state: State, tabId: number, blockType: BlockTypes, subresource: string) => State

export type SaveCosmeticFilterRuleExceptions = (state: State, tabId: number, exceptions: string[]) => State

export type ResetBlockingStats = (state: State, tabId: number) => State

export type ResetBlockingResources = (state: State, tabId: number) => State

export type ResetNoScriptInfo = (state: State, tabId: number, newOrigin: string) => State

export type UpdateShieldsIconBadgeText = (state: State) => void
export type UpdateShieldsIconImage = (state: State) => void
export type UpdateShieldsIcon = (state: State) => void
export type FocusedWindowChanged = (state: State, windowId: number) => State
export type RequestDataAndUpdateActiveTab = (state: State, windowId: number, tabId: number) => State

export type PersistAllNoScriptSettings = (state: State, tabId: number) => State
