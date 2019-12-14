
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as sinon from 'sinon'
import * as cosmeticFilterAPI from '../../../../brave_extension/extension/brave_extension/background/api/cosmeticFilterAPI'

describe('cosmeticFilter API', () => {
  describe('addSiteCosmeticFilter', () => {
    const url = 'https://www.brave.com'
    const filter = '#cssFilter'
    let getStorageStub: any
    let setStorageStub: any
    beforeAll(() => {
      getStorageStub = sinon.stub(chrome.storage.local, 'get')
      setStorageStub = sinon.stub(chrome.storage.local, 'set')
    })
    afterAll(() => {
      getStorageStub.restore()
      setStorageStub.restore()
    })
    beforeEach(() => {
      getStorageStub.resetHistory()
      setStorageStub.resetHistory()
    })

    it('passes only 1 arg to chrome.storage.local.set', () => {
      getStorageStub.yields({
        'list': {
          'hostname': ['samplefilter']
        }
      })
      cosmeticFilterAPI.addSiteCosmeticFilter(url, filter)
        .catch(() => {
          expect(true).toBe(false)
        })
      expect(setStorageStub.getCall(0).args.length).toBe(1)
    })
    it('passes the correct arguments to chrome.storage.local.set when storage is empty', () => {
      getStorageStub.yields({})
      cosmeticFilterAPI.addSiteCosmeticFilter(url, filter)
        .catch(() => {
          expect(true).toBe(false)
        })
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {
          'https://www.brave.com': ['#cssFilter']
        }
      })
    })
    it('passes the correct arguments to chrome.storage.local.set when storage is undefined', () => {
      getStorageStub.yields(undefined)
      cosmeticFilterAPI.addSiteCosmeticFilter(url, filter)
        .catch(() => {
          expect(true).toBe(false)
        })
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {
          'https://www.brave.com': ['#cssFilter']
        }
      })
    })
    it('can add more than 1 filter', () => {
      getStorageStub.yields({
        'cosmeticFilterList': {
          'hostname': ['samplefilter']
        }
      })
      cosmeticFilterAPI.addSiteCosmeticFilter('hostname', 'samplefilter2')
        .catch(() => {
          expect(true).toBe(false)
        })
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        'cosmeticFilterList': {
          'hostname': ['samplefilter', 'samplefilter2']
        }
      })
    })
  })
  describe('removeSiteFilter', () => {
    const url = 'https://www.brave.com'
    const filter = '#cssFilter'
    let getStorageStub: any
    let setStorageStub: any

    beforeAll(() => {
      getStorageStub = sinon.stub(chrome.storage.local, 'get')
      setStorageStub = sinon.stub(chrome.storage.local, 'set')
    })
    afterAll(() => {
      getStorageStub.restore()
      setStorageStub.restore()
    })
    beforeEach(() => {
      getStorageStub.resetHistory()
      setStorageStub.resetHistory()
    })

    it('passes only 1 arg to chrome.storage.local.set', () => {
      getStorageStub.yields({
        'cosmeticFilterList': {
          url: filter
        }
      })
      cosmeticFilterAPI.removeSiteFilter(url)
      expect(setStorageStub.getCall(0).args.length).toBe(1)
    })
    it('removes the correct filter', () => {
      getStorageStub.yields({
        cosmeticFilterList: {
          'https://www.brave.com': ['#cssFilter'],
          'https://notbrave.com': ['notACSSFilter']
        }
      })
      cosmeticFilterAPI.removeSiteFilter(url)
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {
          'https://notbrave.com': ['notACSSFilter']
        }
      })
    })
    it('handles empty storage', () => {
      getStorageStub.yields({})
      cosmeticFilterAPI.removeSiteFilter(url)
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {}
      })
    })
    it('handles undefined storage', () => {
      getStorageStub.yields(undefined)
      cosmeticFilterAPI.removeSiteFilter(url)
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {}
      })
    })
    it('handles url not in storage', () => {
      getStorageStub.yields({
        cosmeticFilterList: {
          url: filter
        }
      })
      cosmeticFilterAPI.removeSiteFilter('urlNotInStorage')
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {
          url: filter
        }
      })
    })
  })
  describe('removeAllFilters', () => {
    let getStorageStub: any
    let setStorageStub: any
    beforeAll(() => {
      getStorageStub = sinon.stub(chrome.storage.local, 'get')
      setStorageStub = sinon.stub(chrome.storage.local, 'set')
    })

    afterAll(() => {
      getStorageStub.restore()
      setStorageStub.restore()
    })
    beforeEach(() => {
      getStorageStub.resetHistory()
      setStorageStub.resetHistory()
    })

    it('sets empty list object', () => {
      getStorageStub.yields({
        cosmeticFilterList: {
          'hostname': 'isNotEmpty'
        }
      })
      cosmeticFilterAPI.removeAllFilters()
      expect(setStorageStub.getCall(0).args[0]).toEqual({
        cosmeticFilterList: {}
      })
    })
  })
  describe('applyCSSCosmeticFilters', () => {
    const filter = '#cssFilter'
    const filter2 = '#cssFilter2'

    let getStorageStub: any
    let setStorageStub: any
    let insertCSSStub: any

    beforeAll(() => {
      getStorageStub = sinon.stub(chrome.storage.local, 'get')
      setStorageStub = sinon.stub(chrome.storage.local, 'set')
      insertCSSStub = sinon.stub(chrome.tabs, 'insertCSS')
    })
    afterAll(() => {
      getStorageStub.restore()
      setStorageStub.restore()
      insertCSSStub.restore()
    })
    beforeEach(() => {
      getStorageStub.resetHistory()
      setStorageStub.resetHistory()
      insertCSSStub.resetHistory()
    })
    it('applies the correct filter', () => {
      getStorageStub.yields({
        cosmeticFilterList: {
          'brave.com': [filter]
        }
      })
      cosmeticFilterAPI.applyCSSCosmeticFilters(1, 'brave.com')
      expect(insertCSSStub.getCall(0).args[0]).toEqual(1)
      expect(insertCSSStub.getCall(0).args[1]).toEqual({
        code: `${filter} {display: none !important;}`,
        cssOrigin: 'user',
        runAt: 'document_start'
      })
    })
    it('applies multiple filters correctly', () => {
      getStorageStub.yields({
        cosmeticFilterList: {
          'brave.com': [filter, filter2]
        }
      })
      cosmeticFilterAPI.applyCSSCosmeticFilters(1, 'brave.com')
      expect(insertCSSStub.getCall(0).args[0]).toEqual(1)
      expect(insertCSSStub.getCall(0).args[1]).toEqual({
        code: `${filter } {display: none !important;}`,
        cssOrigin: 'user',
        runAt: 'document_start'
      })
      expect(insertCSSStub.getCall(1).args[0]).toEqual(1)
      expect(insertCSSStub.getCall(1).args[1]).toEqual({
        code: `${ filter2 } {display: none !important;}`,
        cssOrigin: 'user',
        runAt: 'document_start'
      })
    })
    // chrome.local.storage.get() always returns an empty object if nothing exists
    it('doesn\'t apply filters if storage for host is implicitly undefined', () => {
      getStorageStub.yields({
        cosmeticFilterList: {}
      })
      cosmeticFilterAPI.applyCSSCosmeticFilters(1, 'brave.com')
      expect(insertCSSStub.called).toBe(false)
    })
    it('doesn\'t apply filters if storage is explicitly undefined', () => {
      getStorageStub.yields({
        cosmeticFilterList: {
          'brave.com': undefined
        }
      })
      cosmeticFilterAPI.applyCSSCosmeticFilters(1, 'brave.com')
      expect(insertCSSStub.called).toBe(false)
    })
  })
})
