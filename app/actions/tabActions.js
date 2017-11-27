/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/tabTypes'

export function activeTabChanged (windowId, tabId) {
  return { type: types.ACTIVE_TAB_CHANGED, windowId, tabId }
}

export function tabCreated (tab) {
  return { type: types.TAB_CREATED, tab }
}

export function tabDataChanged (tabId, changeInfo, tab) {
  return { type: types.TAB_DATA_CHANGED, tabId, changeInfo, tab }
}
