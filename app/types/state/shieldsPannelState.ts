/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { BlockOptions, BlockTypes } from '../other/blockTypes'

export interface Tab {
  adBlock: BlockOptions
  adsBlocked: number
  adsTrackers: BlockOptions
  controlsOpen: boolean
  hostname: string
  httpsEverywhere: BlockOptions
  httpsEverywhereRedirected: number
  id: number
  javascript: BlockOptions
  javascriptBlocked: number
  origin: string
  shieldsEnabled: BlockOptions
  trackingProtection: BlockOptions
  trackingProtectionBlocked: number
  url: string
}

export interface Tabs {
  [key: number]: Tab
}

export interface Windows {
  [key: number]: number
}

export interface State {
  currentWindowId: number
  tabs: Tabs
  windows: Windows
}

export interface GetActiveTabId {
  (state: State): number
}

export interface GetActiveTabData {
  (state: State): Tab
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
  (state: State, tabId: number, blockType: BlockTypes): State
}

export interface ResetBlockingStats {
  (state: State, tabId: number): State
}
