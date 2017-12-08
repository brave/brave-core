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

export type webNavigationActions =
  OnBeforeNavigateReturn
