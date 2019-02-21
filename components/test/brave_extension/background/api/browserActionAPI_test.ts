/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as browserActionAPI from '../../../../brave_extension/extension/brave_extension/background/api/browserActionAPI'

describe('BrowserAction API', () => {
  describe('setBadgeText', () => {
    let spy: jest.SpyInstance
    const text = '42'
    const tabId = 1337
    beforeEach(() => {
      spy = jest.spyOn(chrome.browserAction, 'setBadgeText')
      browserActionAPI.setBadgeText(tabId, text)
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.browserAction.setBadgeText with the text', () => {
      expect(spy).toHaveBeenCalled()
      expect(spy.mock.calls[0][0]).toEqual({
        tabId,
        text
      })
    })
  })
  describe('setIcon', () => {
    let setIconSpy: jest.SpyInstance
    let disableSpy: jest.SpyInstance
    let enableSpy: jest.SpyInstance
    let url = 'https://brave.com'
    const tabId = 1
    let shieldsEnabled = true
    beforeEach(() => {
      setIconSpy = jest.spyOn(chrome.browserAction, 'setIcon')
      disableSpy = jest.spyOn(chrome.browserAction, 'disable')
      enableSpy = jest.spyOn(chrome.browserAction, 'enable')
    })
    afterEach(() => {
      setIconSpy.mockRestore()
      disableSpy.mockRestore()
      enableSpy.mockRestore()
    })

    it('sets enabled when protocol is http', () => {
      url = 'http://not-very-awesome-http-page.com'
      browserActionAPI.setIcon(url, tabId, shieldsEnabled)
      expect(enableSpy.mock.calls[0][0]).toBe(tabId)
    })
    it('sets the enabled icon when protocol is https', () => {
      url = 'https://very-awesome-https-page.com'
      browserActionAPI.setIcon(url, tabId, shieldsEnabled)
      expect(enableSpy.mock.calls[0][0]).toBe(tabId)
    })
    it('sets the disabled icon when the protocol is neither https nor http', () => {
      url = 'brave://welcome'
      browserActionAPI.setIcon(url, tabId, shieldsEnabled)
      expect(disableSpy.mock.calls[0][0]).toBe(tabId)
    })
    it('sets the disabled icon when the protocol is http and shield is off', () => {
      url = 'http://not-very-awesome-http-page.com'
      shieldsEnabled = false
      browserActionAPI.setIcon(url, tabId, shieldsEnabled)
      expect(enableSpy.mock.calls[0][0]).toBe(tabId)
      expect(setIconSpy.mock.calls[0][0]).toEqual({
        path: browserActionAPI.shieldsOffIcon,
        tabId
      })
    })
    it('sets the disabled icon when the protocol is https and shield is off', () => {
      url = 'https://very-awesome-https-page.com'
      shieldsEnabled = false
      browserActionAPI.setIcon(url, tabId, shieldsEnabled)
      expect(enableSpy.mock.calls[0][0]).toBe(tabId)
      expect(setIconSpy.mock.calls[0][0]).toEqual({
        path: browserActionAPI.shieldsOffIcon,
        tabId
      })
    })
  })
})
