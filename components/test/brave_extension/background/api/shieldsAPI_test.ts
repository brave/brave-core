/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import actions from '../../../../brave_extension/extension/brave_extension/background/actions/shieldsPanelActions'
import * as shieldsAPI from '../../../../brave_extension/extension/brave_extension/background/api/shieldsAPI'
// import { Tab as TabType } from '../../../types/state/shieldsPannelState'
import * as resourceIdentifiers from '../../../../brave_extension/extension/brave_extension/constants/resourceIdentifiers'
import {activeTabData} from '../../../testData'

describe('Shields API', () => {
  describe('getShieldSettingsForTabData', () => {
    it('returns a rejected promise when no tab data is specified', (cb) => {
      shieldsAPI.getShieldSettingsForTabData(undefined)
        .catch(() => {
          cb()
        })
    })

    it('resolves the returned promise with shield settings for the tab data', (cb) => {
      const tab: chrome.tabs.Tab = {
        url: 'https://www.brave.com/serg/dont/know/pokemon',
        id: 5,
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false
      }

      expect.assertions(1)
      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        expect(data).toEqual({
          url: 'https://www.brave.com/serg/dont/know/pokemon',
          origin: 'https://www.brave.com',
          hostname: 'www.brave.com',
          braveShields: 'block',
          ads: 'block',
          trackers: 'block',
          httpUpgradableResources: 'block',
          javascript: 'block',
          fingerprinting: 'block',
          id: 5,
          cookies: 'block'
        })
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })

    it('returns `block` by default for braveShields', (cb) => {
      const tab: chrome.tabs.Tab = {
        url: 'https://www.brave.com/charizard/knows/serg',
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false,
        id: 1337
      }

      expect.assertions(1)
      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        expect(data.braveShields).toBe('block')
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })

    it('returns `block` by default for braveShields when origin is not http or https', (cb) => {
      const tab: chrome.tabs.Tab = {
        url: 'ftp://www.brave.com/serg/dont/know/pikachu',
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false,
        id: 1337
      }

      expect.assertions(1)
      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        expect(data.braveShields).toBe('block')
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })

    it('returns `block` by default for braveShields when origin is an about page', (cb) => {
      const tab: chrome.tabs.Tab = {
        url: 'chrome://welcome',
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false,
        id: 1337
      }

      expect.assertions(1)
      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        expect(data.braveShields).toBe('block')
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })
  })

  describe('getTabData', () => {
    let spy: jest.SpyInstance
  const tabId = 2
  beforeAll(() => {spy = jest.spyOn(chrome.tabs, 'getAsync')})
  afterEach(() => {spy.mockRestore()})

    it('calls chrome.tabs.getAsync for the active tab', () => {
      expect.assertions(1)
    shieldsAPI.getTabData(tabId)
      expect(spy).toBeCalledWith(tabId)
    })

    it('resolves the promise with an array', (cb) => {
      expect.assertions(1)
      shieldsAPI.getTabData(tabId)
      .then((tab: chrome.tabs.Tab) => {
        expect(tab).toEqual(activeTabData)
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })
  })

  describe('requestShieldPanelData', () => {
    let spy: jest.SpyInstance
  const tabId = 2
  beforeEach(() => {spy = jest.spyOn(actions, 'shieldsPanelDataUpdated')})
  afterEach(() => {spy.mockRestore()})
    it('resolves and calls requestShieldPanelData', (cb) => {
      const tab = {
        url: 'https://www.brave.com/test',
        origin: 'https://www.brave.com',
        hostname: 'www.brave.com',
        id: 2,
        braveShields: 'block',
        ads: 'block',
        trackers: 'block',
        httpUpgradableResources: 'block',
        javascript: 'block',
        fingerprinting: 'block',
        cookies: 'block'
      }

      expect.assertions(2)
      shieldsAPI.requestShieldPanelData(tabId)
        .then(() => {
          expect(spy).toBeCalledTimes(1)
      expect(spy.mock.calls[0][0]).toEqual(tab)
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowAds', () => {
    let spy: jest.SpyInstance
  beforeEach(() => {spy = jest.spyOn(chrome.braveShields.plugins, 'setAsync')})
  afterEach(() => {spy.mockRestore()})
    it('calls chrome.braveShields.plugins with the correct args', () => {
      shieldsAPI.setAllowAds('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(1)
      expect(arg0).toEqual({
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_ADS },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', () => {
      shieldsAPI.setAllowAds('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(1)
      expect(spy.mock.calls[0].length).toBe(1)
    })
    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowAds('https://www.brave.com', 'block')
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowTrackers', () => {
    let spy: jest.SpyInstance
  beforeEach(() => {spy = jest.spyOn(chrome.braveShields.plugins, 'setAsync')})
  afterEach(() => {spy.mockRestore()})
    it('calls chrome.braveShields.plugins with the correct args', () => {
      shieldsAPI.setAllowTrackers('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(1)
      expect(arg0).toEqual({
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_TRACKERS },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', () => {
      shieldsAPI.setAllowTrackers('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(1)
      expect(spy.mock.calls[0].length).toBe(1)
    })
    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowTrackers('https://www.brave.com', 'block')
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowHTTPUpgradableResource', () => {
    let spy: jest.SpyInstance
  beforeEach(() => {spy = jest.spyOn(chrome.braveShields.plugins, 'setAsync')})
  afterEach(() => {spy.mockRestore()})
    it('calls chrome.braveShields.plugins with the correct args', () => {
      shieldsAPI.setAllowHTTPUpgradableResources('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(1)
      expect(arg0).toEqual({
        primaryPattern: '*://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', () => {
      shieldsAPI.setAllowHTTPUpgradableResources('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(1)
      expect(spy.mock.calls[0].length).toBe(1)
    })
    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowHTTPUpgradableResources('https://www.brave.com', 'block')
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowJavaScript', () => {
    let spy: jest.SpyInstance
  beforeEach(
      () => {spy = jest.spyOn(chrome.braveShields.javascript, 'setAsync')})
  afterEach(() => {spy.mockRestore()})

    it('calls chrome.braveShields.plugins with the correct args', () => {
      shieldsAPI.setAllowJavaScript('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(1)
      expect(arg0).toEqual({
        primaryPattern: 'https://www.brave.com/*',
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })

    it('passes only 1 arg to chrome.braveShields.plugins', () => {
      shieldsAPI.setAllowJavaScript('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(1)
      expect(spy.mock.calls[0].length).toBe(1)
    })

    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowJavaScript('https://www.brave.com', 'block')
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowFingerprinting', () => {
    let spy: jest.SpyInstance
  beforeEach(() => {spy = jest.spyOn(chrome.braveShields.plugins, 'setAsync')})
  afterEach(() => {spy.mockRestore()})
    it('calls chrome.braveShields.plugins with the correct args', () => {
      shieldsAPI.setAllowFingerprinting('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(2)
    expect(arg0).toEqual({
      primaryPattern: 'https://www.brave.com/*',
      resourceIdentifier:
          {id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING},
      setting: 'block',
      scope: 'incognito_session_only'
    })
    const arg1 = spy.mock.calls[1][0]
      expect(arg1).toEqual({
        primaryPattern: 'https://www.brave.com/*',
        secondaryPattern: 'https://firstParty/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', () => {
      shieldsAPI.setAllowFingerprinting('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(2)
    expect(spy.mock.calls[0].length).toBe(1)
      expect(spy.mock.calls[1].length).toBe(1)
    })
    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowFingerprinting('https://www.brave.com', 'block')
        .then(function () {
  cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowCookies', () => {
    let spy: jest.SpyInstance
  beforeEach(() => {spy = jest.spyOn(chrome.braveShields.plugins, 'setAsync')})
  afterEach(() => {spy.mockRestore()})
    it('calls chrome.braveShields.plugins with the correct args', () => {
      shieldsAPI.setAllowCookies('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(3)
    expect(arg0).toEqual({
      primaryPattern: 'https://www.brave.com/*',
      resourceIdentifier:
          {id: resourceIdentifiers.RESOURCE_IDENTIFIER_REFERRERS},
      setting: 'block',
      scope: 'incognito_session_only'
    })
    const arg1 = spy.mock.calls[1][0]
    expect(arg1).toEqual({
      primaryPattern: 'https://www.brave.com/*',
      resourceIdentifier: {id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES},
      setting: 'block',
      scope: 'incognito_session_only'
    })
    const arg2 = spy.mock.calls[2][0]
      expect(arg2).toEqual({
        primaryPattern: 'https://www.brave.com/*',
        secondaryPattern: 'https://firstParty/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', () => {
      shieldsAPI.setAllowCookies('https://www.brave.com', 'block')
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(2)
    expect(spy.mock.calls[0].length).toBe(1)
      expect(spy.mock.calls[1].length).toBe(1)
    })
    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowCookies('https://www.brave.com', 'block')
        .then(() => {
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('toggleShieldsValue', () => {
    it('toggles \'allow\' to \'block\'', () => {
      expect(shieldsAPI.toggleShieldsValue('allow')).toBe('block')
    })
    it('toggles \'block\' to \'allow\'', () => {
      expect(shieldsAPI.toggleShieldsValue('block')).toBe('allow')
    })
  })

  describe('setAllowScriptOriginsOnce', () => {
    let spy: jest.SpyInstance
  beforeEach(() => {spy = jest.spyOn(chrome.braveShields, 'allowScriptsOnce')})
  afterEach(() => {spy.mockRestore()})
    it('calls chrome.braveShields.allowScriptsOnce with the correct args', () => {
      shieldsAPI.setAllowScriptOriginsOnce(['https://a.com/', 'https://b.com/'], 2)
        .catch(() => {
          expect(true).toBe(false)
        })
    const arg0 = spy.mock.calls[0][0]
    expect.assertions(2)
    expect(arg0).toEqual(['https://a.com/', 'https://b.com/'])
    const arg1 = spy.mock.calls[0][1]
      expect(arg1).toBe(2)
    })
    it('passes 3 args to chrome.braveShields.allowScriptsOnce', () => {
      shieldsAPI.setAllowScriptOriginsOnce(['https://a.com/', 'https://b.com/'], 2)
        .catch(() => {
          expect(true).toBe(false)
        })
    expect.assertions(1)
      expect(spy.mock.calls[0].length).toBe(3) // include callback
    })
    it('resolves the returned promise', (cb) => {
      shieldsAPI.setAllowScriptOriginsOnce(['https://a.com/', 'https://b.com/'], 2)
      .then(() => {
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })
  })
})
