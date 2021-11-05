/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import { NoScriptInfo } from '../../../brave_extension/extension/brave_extension/types/other/noScriptInfo'
import { State } from '../../../brave_extension/extension/brave_extension/types/state/shieldsPannelState'

// State helpers
import * as noScriptState from '../../../brave_extension/extension/brave_extension/state/noScriptState'

// Helpers
import deepFreeze from '../../deepFreeze'
import { getHostname, getOrigin } from '../../../brave_extension/extension/brave_extension/helpers/urlUtils'

const url1: string = 'http://aaaa.com/script1.js'
const url2: string = 'http://aaaa.com/script2.js'
const url3: string = 'http://bbbb.com/script1.js'
const url4: string = 'http://cccc.com/script1.js'
const url5: string = 'http://dddd.com/script1.js'

const noScriptInfo: NoScriptInfo = {
  [url1]: { actuallyBlocked: false, willBlock: false, userInteracted: false },
  [url2]: { actuallyBlocked: false, willBlock: false, userInteracted: false },
  [url3]: { actuallyBlocked: false, willBlock: true, userInteracted: false },
  [url4]: { actuallyBlocked: false, willBlock: true, userInteracted: false },
  [url5]: { actuallyBlocked: true, willBlock: true, userInteracted: false }
}

const url: string = 'https://brave.com'
const tabId: number = 2
const state: State = deepFreeze({
  tabs: { [tabId]: {
    origin: url,
    id: tabId,
    noScriptInfo ,
    hostname: 'https://brave.com',
    url: 'https://brave.com',
    ads: 'block',
    adsBlocked: 0,
    adsBlockedResources: [],
    braveShields: 'allow',
    controlsOpen: true,
    trackers: 'block',
    trackersBlocked: 0,
    trackersBlockedResources: [],
    httpUpgradableResources: 'block',
    httpsRedirected: 0,
    httpsRedirectedResources: [],
    javascript: 'block',
    javascriptBlocked: 0,
    fingerprinting: 'block',
    fingerprintingBlocked: 0,
    fingerprintingBlockedResources: [],
    cookies: 'block',
    firstPartyCosmeticFiltering: false
  } },
  windows: { 1: tabId },
  persistentData: {
    isFirstAccess: true
  },
  currentWindowId: 1,
  settingsData: {
    showAdvancedView: true,
    statsBadgeVisible: true
  }
})

describe('noScriptState', () => {
  describe('getNoScriptInfo', () => {
    it('gets noScriptInfo data defined in state', () => {
      const assertion = noScriptState.getNoScriptInfo(state, tabId)
      expect(assertion).toBe(noScriptInfo)
    })
  })
  describe('modifyNoScriptInfo', () => {
    it('modifies noScript data', () => {
      const url = 'https://awesome-new-website-for-testing-purposes.net'
      const assertion = noScriptState.modifyNoScriptInfo(state, tabId, url, {
        actuallyBlocked: true,
        willBlock: true,
        userInteracted: true
      })
      expect(assertion).toEqual({ ...state, tabs: {
        2: {
          ...state.tabs[2],
          noScriptInfo: {
            ...state.tabs[2].noScriptInfo,
            'https://awesome-new-website-for-testing-purposes.net': {
              actuallyBlocked: true,
              userInteracted: true,
              willBlock: true
            }
          }
        }}
      })
    })
  })

  describe('setScriptBlockedCurrentState', () => {
    it('set userInteracted to true', () => {
      const modifiedState = noScriptState.setScriptBlockedCurrentState(state, url1)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url1].userInteracted
      expect(assertion).toBe(true)
    })

    it('set actuallyBlocked to false if its true', () => {
      const modifiedState = noScriptState.setScriptBlockedCurrentState(state, url5)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url1].actuallyBlocked
      expect(assertion).toBe(false)
    })

    it('set actuallyBlocked to true if its false', () => {
      const modifiedState = noScriptState.setScriptBlockedCurrentState(state, url1)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url1].actuallyBlocked
      expect(assertion).toBe(true)
    })
  })

  describe('setGroupedScriptsBlockedCurrentState', () => {
    it('toggle userInteracted if hostname match and willBlock is the same as maybeBlock', () => {
      const hostname = getOrigin(url3)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, true)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url3].userInteracted
      expect(assertion).toBe(true)
    })

    it('does not toggle userInteracted if hostname match and willBlock is not the same as maybeBlock', () => {
      const hostname = getHostname(url3)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, false)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url3].userInteracted
      expect(assertion).toBe(false)
    })

    it('toggle actuallyBlocked when willBlock is the same as maybeBlock', () => {
      const hostname = getOrigin(url5)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, true)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url5].actuallyBlocked
      expect(assertion).toBe(false)
    })

    it('does not toggle actuallyBlocked when willBlock is not the same as maybeBlock', () => {
      const hostname = getOrigin(url5)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, false)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url5].actuallyBlocked
      expect(assertion).toBe(true)
    })

    it('toggle actuallyBlocked when willBlock is the same as maybeBlock', () => {
      const hostname = getOrigin(url4)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, true)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url4].actuallyBlocked
      expect(assertion).toBe(true)
    })

    it('does not toggle actuallyBlocked when willBlock is not the same as maybeBlock', () => {
      const hostname = getOrigin(url4)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, false)
      const assertion = modifiedState.tabs[tabId].noScriptInfo[url4].actuallyBlocked
      expect(assertion).toBe(false)
    })

    it('does not modify state if hostname does not match', () => {
      const url = 'https://malicious-scripts-strike-back.com'
      const hostname = getOrigin(url)
      const modifiedState = noScriptState.setGroupedScriptsBlockedCurrentState(state, hostname, true)
      const assertion = modifiedState
      expect(assertion).toEqual(state)
    })
  })

  describe('setAllScriptsBlockedCurrentState', () => {
    it('set all userInteracted to true when willBlock is different from maybeBlock', () => {
      const willBlock = false
      const maybeBlock = true
      const newStateWithNoScriptInfo = Object.create({
        windows: { 1: 2 },
        currentWindowId: 1,
        tabs: {
          ...state.tabs,
          [tabId]: {
            ...state.tabs[tabId],
            noScriptInfo: {
              [url1]: { willBlock, userInteracted: false, actuallyBlocked: true },
              [url2]: { willBlock, userInteracted: false, actuallyBlocked: true }
            }
          }
        }
      })
      const modifiedState = noScriptState.setAllScriptsBlockedCurrentState(newStateWithNoScriptInfo, maybeBlock)
      const assertion = Object.entries(modifiedState.tabs[tabId].noScriptInfo)
        .every(script => script[1].userInteracted)
      expect(assertion).toBe(true)
    })

    it('set all actuallyBlocked to be the same as maybeBlock when willBlock is different', () => {
      const willBlock = false
      const maybeBlock = true
      const newStateWithNoScriptInfo = Object.create({
        windows: { 1: 2 },
        currentWindowId: 1,
        tabs: {
          ...state.tabs,
          [tabId]: {
            ...state.tabs[tabId],
            noScriptInfo: {
              [url1]: { willBlock, userInteracted: false, actuallyBlocked: true },
              [url2]: { willBlock, userInteracted: false, actuallyBlocked: true }
            }
          }
        }
      })
      const modifiedState = noScriptState.setAllScriptsBlockedCurrentState(newStateWithNoScriptInfo, maybeBlock)
      const assertion = Object.entries(modifiedState.tabs[tabId].noScriptInfo)
        .every(script => script[1].actuallyBlocked)
      expect(assertion).toBe(maybeBlock)
    })

    it('does not modify actuallyBlocked when maybeBlock is the same as willBlock', () => {
      const willBlock = false
      const maybeBlock = false
      const newStateWithNoScriptInfo = Object.create({
        windows: { 1: 2 },
        currentWindowId: 1,
        tabs: {
          ...state.tabs,
          [tabId]: {
            ...state.tabs[tabId],
            noScriptInfo: {
              [url2]: { willBlock, userInteracted: false, actuallyBlocked: false },
              [url2]: { willBlock, userInteracted: false, actuallyBlocked: false }
            }
          }
        }
      })
      const modifiedState = noScriptState.setAllScriptsBlockedCurrentState(newStateWithNoScriptInfo, maybeBlock)
      const assertion = Object.entries(modifiedState.tabs[tabId].noScriptInfo)
        .every(script => script[1].actuallyBlocked)
      expect(assertion).toBe(maybeBlock)
    })
  })

  describe('setFinalScriptsBlockedState', () => {
    it('set all userInteracted to false', () => {
      const newStateWithNoScriptInfo = Object.create({
        windows: { 1: 2 },
        currentWindowId: 1,
        tabs: {
          ...state.tabs,
          [tabId]: {
            ...state.tabs[tabId],
            noScriptInfo: {
              [url2]: { ...state.tabs[tabId].noScriptInfo[url2], userInteracted: true },
              [url2]: { ...state.tabs[tabId].noScriptInfo[url2], userInteracted: true }
            }
          }
        }
      })
      const modifiedState = noScriptState.setFinalScriptsBlockedState(newStateWithNoScriptInfo)
      const assertion = Object.entries(modifiedState.tabs[tabId].noScriptInfo)
        .every(script => script[1].userInteracted)
      expect(assertion).toBe(false)
    })

    it('set all willBlock properties to be true if actuallyBlocked is true', () => {
      const newStateWithNoScriptInfo = Object.create({
        windows: { 1: 2 },
        currentWindowId: 1,
        tabs: {
          ...state.tabs,
          [tabId]: {
            ...state.tabs[tabId],
            noScriptInfo: {
              [url2]: { ...state.tabs[tabId].noScriptInfo[url2], actuallyBlocked: true },
              [url2]: { ...state.tabs[tabId].noScriptInfo[url2], actuallyBlocked: true }
            }
          }
        }
      })
      const modifiedState = noScriptState.setFinalScriptsBlockedState(newStateWithNoScriptInfo)
      const assertion = Object.entries(modifiedState.tabs[tabId].noScriptInfo)
        .every(script => script[1].willBlock)
      expect(assertion).toBe(true)
    })

    it('set all willBlock properties to be false if actuallyBlocked is false', () => {
      const newStateWithNoScriptInfo = Object.create({
        windows: { 1: 2 },
        currentWindowId: 1,
        tabs: {
          ...state.tabs,
          [tabId]: {
            ...state.tabs[tabId],
            noScriptInfo: {
              [url2]: { ...state.tabs[tabId].noScriptInfo[url2], actuallyBlocked: false },
              [url2]: { ...state.tabs[tabId].noScriptInfo[url2], actuallyBlocked: false }
            }
          }
        }
      })
      const modifiedState = noScriptState.setFinalScriptsBlockedState(newStateWithNoScriptInfo)
      const assertion = Object.entries(modifiedState.tabs[tabId].noScriptInfo)
        .every(script => script[1].willBlock)
      expect(assertion).toBe(false)
    })
  })
})
