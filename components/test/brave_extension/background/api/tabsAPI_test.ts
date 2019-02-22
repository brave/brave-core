/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as tabsAPI from '../../../../brave_extension/extension/brave_extension/background/api/tabsAPI'

describe('tabs API', () => {
  describe('createTab', () => {
    let spy: jest.SpyInstance
    const createProperties = { url: 'https://www.brave.com' }
    beforeEach(() => {
      spy = jest.spyOn(chrome.tabs, 'create')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.tabs.create with the createProperties', (cb) => {
      expect.assertions(2)
      tabsAPI.createTab(createProperties)
        .then(() => {
          expect(spy).toBeCalledTimes(1)
          expect(spy.mock.calls[0][0]).toEqual(createProperties)
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })
  describe('reloadTab', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.tabs, 'reload')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.tabs.reload without bypassing the cache', (cb) => {
      const tabId = 42
      const bypassCache = false
      expect.assertions(3)
      tabsAPI.reloadTab(tabId, bypassCache)
        .then(() => {
          expect(spy).toBeCalledTimes(1)
          expect(spy.mock.calls[0][0]).toBe(tabId)
          expect(spy.mock.calls[0][1]).toEqual({ bypassCache })
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
    it('calls chrome.tabs.reload with bypassing the cache', (cb) => {
      const tabId = 42
      const bypassCache = true
      expect.assertions(3)
      tabsAPI.reloadTab(tabId, bypassCache)
        .then(() => {
          expect(spy).toBeCalledTimes(1)
          expect(spy.mock.calls[0][0]).toBe(tabId)
          expect(spy.mock.calls[0][1]).toEqual({ bypassCache })
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })
})
