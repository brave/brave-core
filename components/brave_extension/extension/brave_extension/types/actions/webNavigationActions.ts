/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/webNavigationTypes'

interface OnBeforeNavigateReturn {
  type: types.ON_BEFORE_NAVIGATION,
  tabId: number,
  url: string,
  isMainFrame: boolean
}

export interface OnBeforeNavigate {
  (tabId: number, url: string, isMainFrame: boolean): OnBeforeNavigateReturn
}

interface OnCommittedReturn {
  type: types.ON_COMMITTED,
  tabId: number,
  url: string,
  isMainFrame: boolean
}
interface OnErrorOccurredReturn {
  type: types.ON_ERROR_OCCURRED,
  error: string,
  tabId: number,
  isMainFrame: boolean
}
export interface OnCommitted {
  (tabId: number, url: string, isMainFrame: boolean): OnCommittedReturn
}
export interface OnErrorOccurred {
  (error: string, tabId: number, isMainFrame: boolean): OnErrorOccurredReturn
}

export type webNavigationActions =
  OnBeforeNavigateReturn |
  OnCommittedReturn |
  OnErrorOccurredReturn
