/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import { Tabs } from '../types/state/shieldsPannelState'
import {
  GetNoScriptInfo,
  ModifyNoScriptInfo,
  ResetNoScriptInfo,
  SetScriptBlockedCurrentState,
  SetGroupedScriptsBlockedCurrentState,
  SetAllScriptsBlockedCurrentState,
  SetFinalScriptsBlockedState
} from '../types/state/noScriptState'

// Helpers
import { getActiveTabId } from './shieldsPanelState'
import { filterNoScriptInfoByWillBlockState } from '../helpers/noScriptUtils'
import { getOrigin } from '../helpers/urlUtils'

/**
 * Get NoScriptInfo initial state
 * @param {State} state - The initial NoScriptState
 * @returns {NoScriptInfo} The current NoScript data
 */
export const getNoScriptInfo: GetNoScriptInfo = (state, tabId) => {
  if (!('noScriptInfo' in state.tabs[tabId])) {
    state.tabs[tabId].noScriptInfo = {}
  }
  return state.tabs[tabId].noScriptInfo
}

/**
 * Set NoScriptInfo modified state
 * @param {State} state - The Application state
 * @param {tabId} number - The current tab ID
 * @param {string} url - The current script URL
 * @param {object} modifiedInfo - The current script URL object data to be modified
 * @returns {State} state - The modified application state
 */
export const modifyNoScriptInfo: ModifyNoScriptInfo = (state, tabId, url, modifiedInfo) => {
  const tabs: Tabs = {
    ...state.tabs,
    [tabId]: {
      ...state.tabs[tabId],
      noScriptInfo: {
        ...state.tabs[tabId].noScriptInfo,
        [url]: {
          ...state.tabs[tabId].noScriptInfo[url],
          ...modifiedInfo
        }
      }
    }
  }
  return { ...state, tabs }
}

/**
 * Reset NoScriptInfo to its initial state
 * @param {State} state - The Application state
 * @param {number} tabId - The current tab ID
 * @returns {State} The modified application state
 */
export const resetNoScriptInfo: ResetNoScriptInfo = (state, tabId, newOrigin) => {
  const tabs: Tabs = { ...state.tabs }

  if (newOrigin !== tabs[tabId].origin) { // navigate away
    tabs[tabId].noScriptInfo = {}
  }
  return { ...state, tabs }
}

/**
 * Set current script to be either blocked or allowed
 * @param {State} state - The Application state
 * @param {string} url - The current script URL
 * @returns {State} The modified application state
 */
export const setScriptBlockedCurrentState: SetScriptBlockedCurrentState = (state, url) => {
  const tabId: number = getActiveTabId(state)
  const noScriptInfo = getNoScriptInfo(state, tabId)

  const scriptBlockedState = {
    userInteracted: true,
    actuallyBlocked: !noScriptInfo[url].actuallyBlocked
  }
  state = modifyNoScriptInfo(state, tabId, url, scriptBlockedState)
  return state
}

/**
 * Set all scripts in a group to be either blocked or allowed
 * @param {State} state - The Application state
 * @param {string} origin - The current script URL origin
 * @returns {State} The modified application state
 */
export const setGroupedScriptsBlockedCurrentState: SetGroupedScriptsBlockedCurrentState = (
  state,
  origin,
  maybeBlock
) => {
  const tabId = getActiveTabId(state)
  const noScriptInfo = getNoScriptInfo(state, tabId)
  const groupedScripts = Object.entries(noScriptInfo)

  for (const [url] of filterNoScriptInfoByWillBlockState(groupedScripts, maybeBlock)) {
    const groupedScriptsBlockedState = {
      userInteracted: true,
      actuallyBlocked: !noScriptInfo[url].actuallyBlocked
    }

    if (origin === getOrigin(url)) {
      state = modifyNoScriptInfo(state, tabId, url, groupedScriptsBlockedState)
    }
  }
  return state
}

/**
 * Set all scripts to be either blocked or allowed
 * @param {State} state - The Application state
 * @returns {State} The modified application state
 */
export const setAllScriptsBlockedCurrentState: SetAllScriptsBlockedCurrentState = (
  state,
  maybeBlock
) => {
  const tabId = getActiveTabId(state)
  const noScriptInfo = getNoScriptInfo(state, tabId)
  const allBlockedScripts = Object.entries(noScriptInfo)

  for (const [url] of filterNoScriptInfoByWillBlockState(allBlockedScripts, !maybeBlock)) {
    const groupedScriptsBlockedState = {
      userInteracted: true,
      actuallyBlocked: maybeBlock
    }
    state = modifyNoScriptInfo(state, tabId, url, groupedScriptsBlockedState)
  }
  return state
}

/**
 * Set all scripts to be either blocked or allowed after user finish
 * @param {State} state - The Application state
 * @returns {State} The modified application state
 */
export const setFinalScriptsBlockedState: SetFinalScriptsBlockedState = (state) => {
  const tabId = getActiveTabId(state)
  const noScriptInfo = getNoScriptInfo(state, tabId)
  const allScripts = Object.entries(noScriptInfo)

  for (const [url] of allScripts) {
    // `willBlock` set the state once user close Shields panel moving blocked/allowed resources
    // to their respective lists. In this case we copy data from actuallyBlocked and apply
    // the sate state to willBlock so users can see their scripts list updated.
    const groupedScriptsBlockedState = {
      userInteracted: false,
      willBlock: noScriptInfo[url].actuallyBlocked
    }
    state = modifyNoScriptInfo(state, tabId, url, groupedScriptsBlockedState)
  }
  return state
}
