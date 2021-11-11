/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { State } from '../state/shieldsPannelState'
import { NoScriptInfo } from '../other/noScriptInfo'

export type GetNoScriptInfo = (state: State, tabId: number) => NoScriptInfo

export type ModifyNoScriptInfo = (state: State, tabId: number, url: string, modifiedInfo: {}) => State

export type ResetNoScriptInfo = (state: State, tabId: number, newOrigin: string) => State

export type SetScriptBlockedCurrentState = (state: State, url: string) => State

export type SetGroupedScriptsBlockedCurrentState = (state: State, origin: string, maybeBlock: boolean) => State

export type SetAllScriptsBlockedCurrentState = (state: State, maybeBlock: boolean) => State

export type SetFinalScriptsBlockedState = (state: State) => State
