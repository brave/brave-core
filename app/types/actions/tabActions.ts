/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/tabTypes'
import * as state from '../state/shieldsPannelState'

export interface ActiveTabChanged {
  (windowId: number, tabId: number): {
    type: types.ACTIVE_TAB_CHANGED,
    windowId: number,
    tabId: number
  }
}

export interface TabCreated {
  (tab: state.Tab): {
    type: types.TAB_CREATED,
    tab: state.Tab
  }
}

export interface TabDataChanged {
  (tabId: number, changeInfo: state.Tab, tab: state.Tab): {
    type: types.TAB_DATA_CHANGED,
    tabId: number,
    changeInfo: state.Tab,
    tab: state.Tab
  }
}

export type tabActions =
  ActiveTabChanged |
  TabCreated |
  TabDataChanged
