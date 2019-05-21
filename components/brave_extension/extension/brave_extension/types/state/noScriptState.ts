/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { State } from '../state/shieldsPannelState'
import { NoScriptInfo } from '../other/noScriptInfo'

export interface GetNoScriptInfo {
  (state: State, tabId: number): NoScriptInfo
}

export interface ModifyNoScriptInfo {
  (state: State, tabId: number, url: string, modifiedInfo: {}): State
}

export interface SetFinalScriptsBlockedState {
  (state: State): State
}
