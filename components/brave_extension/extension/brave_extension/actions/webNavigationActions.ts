/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/webNavigationTypes'
import * as actions from '../types/actions/webNavigationActions'

export const onBeforeNavigate: actions.OnBeforeNavigate = (tabId, url, isMainFrame) => {
  return {
    type: types.ON_BEFORE_NAVIGATION,
    tabId,
    url,
    isMainFrame
  }
}

export const onCommitted: actions.OnCommitted = (tabId, url, isMainFrame) => {
  return {
    type: types.ON_COMMITTED,
    tabId,
    url,
    isMainFrame
  }
}

export const onErrorOccurred: actions.OnErrorOccurred = (error, tabId, isMainFrame) => {
  return {
    type: types.ON_ERROR_OCCURRED,
    error,
    tabId,
    isMainFrame
  }
}
