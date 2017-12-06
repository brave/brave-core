/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/tabTypes'

// TODO @bbondy what else do we have inhere?
export interface Tab {
  id: number
}

export interface ActiveTabChanged {
  (windowId: number, tabId: number): {
    type: types.ACTIVE_TAB_CHANGED,
    windowId: number,
    tabId: number
  }
}

export interface TabCreated {
  (tab: Tab): {
    type: types.TAB_CREATED,
    tab: Tab
  }
}

export interface TabDataChanged {
  (tabId: number, changeInfo: Tab, tab: Tab): {
    type: types.TAB_DATA_CHANGED,
    tabId: number,
    changeInfo: Tab,
    tab: Tab
  }
}
