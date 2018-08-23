
import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'

import * as cosmeticFilterAPI from '../../../../app/background/api/cosmeticFilterAPI'

describe('cosmeticFilterTestSuite', () => {
  describe('addSiteCosmeticFilter', function () {
    const url = 'https://www.brave.com'
    const filter = '#cssFilter'

    before(function () {
      this.getStorageStub = sinon.stub(chrome.storage.local, 'get')
      this.setStorageStub = sinon.stub(chrome.storage.local, 'set')
    })
    after(function () {
      this.getStorageStub.restore()
      this.setStorageStub.restore()
    })
    beforeEach(function () {
      this.getStorageStub.reset()
      this.setStorageStub.reset()
    })

    it('passes only 1 arg to chrome.storage.local.set', function () {
      this.getStorageStub.yields({
        'list': {
          'hostname': ['samplefilter']
        }
      })
      cosmeticFilterAPI.addSiteCosmeticFilter(url, filter)
      assert.equal(this.setStorageStub.getCall(0).args.length, 1)
    })
    it('passes the correct arguments to chrome.storage.local.set when storage is empty', function () {
      this.getStorageStub.yields({})
      cosmeticFilterAPI.addSiteCosmeticFilter(url, filter)
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {
          'https://www.brave.com': ['#cssFilter']
        }
      })
    })
    it('passes the correct arguments to chrome.storage.local.set when storage is undefined', function () {
      this.getStorageStub.yields(undefined)
      cosmeticFilterAPI.addSiteCosmeticFilter(url, filter)
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {
          'https://www.brave.com': ['#cssFilter']
        }
      })
    })
    it('can add more than 1 filter', function () {
      this.getStorageStub.yields({
        'cosmeticFilterList': {
          'hostname': ['samplefilter']
        }
      })
      cosmeticFilterAPI.addSiteCosmeticFilter('hostname', 'samplefilter2')
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        'cosmeticFilterList': {
          'hostname': ['samplefilter', 'samplefilter2']
        }
      })
    })
  })
  describe('removeSiteFilter', function () {
    const url = 'https://www.brave.com'
    const filter = '#cssFilter'

    before(function () {
      this.getStorageStub = sinon.stub(chrome.storage.local, 'get')
      this.setStorageStub = sinon.stub(chrome.storage.local, 'set')
    })
    after(function () {
      this.getStorageStub.restore()
      this.setStorageStub.restore()
    })
    beforeEach(function () {
      this.getStorageStub.reset()
      this.setStorageStub.reset()
    })
    it('passes only 1 arg to chrome.storage.local.set', function () {
      this.getStorageStub.yields({
        'cosmeticFilterList': {
          url: filter
        }
      })
      cosmeticFilterAPI.removeSiteFilter(url)
      assert.equal(this.setStorageStub.getCall(0).args.length, 1)
    })
    it('removes the correct filter', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {
          'https://www.brave.com': ['#cssFilter'],
          'https://notbrave.com': ['notACSSFilter']
        }
      })
      cosmeticFilterAPI.removeSiteFilter(url)
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {
          'https://notbrave.com': ['notACSSFilter']
        }
      })
    })
    it('handles empty storage', function () {
      this.getStorageStub.yields({})
      cosmeticFilterAPI.removeSiteFilter(url)
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {}
      })
    })
    it('handles undefined storage', function () {
      this.getStorageStub.yields(undefined)
      cosmeticFilterAPI.removeSiteFilter(url)
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {}
      })
    })
    it('handles url not in storage', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {
          url: filter
        }
      })
      cosmeticFilterAPI.removeSiteFilter('urlNotInStorage')
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {
          url: filter
        }
      })
    })
  })
  describe('removeAllFilters', function () {
    before(function () {
      this.getStorageStub = sinon.stub(chrome.storage.local, 'get')
      this.setStorageStub = sinon.stub(chrome.storage.local, 'set')
    })
    after(function () {
      this.getStorageStub.restore()
      this.setStorageStub.restore()
    })
    beforeEach(function () {
      this.getStorageStub.reset()
      this.setStorageStub.reset()
    })

    it('sets empty list object', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {
          'hostname': 'isNotEmpty'
        }
      })
      cosmeticFilterAPI.removeAllFilters()
      assert.deepEqual(this.setStorageStub.getCall(0).args[0], {
        cosmeticFilterList: {}
      })
    })
  })
  describe('applySiteFilters', function () {
    const filter = '#cssFilter'
    const filter2 = '#cssFilter2'

    before(function () {
      this.getStorageStub = sinon.stub(chrome.storage.local, 'get')
      this.setStorageStub = sinon.stub(chrome.storage.local, 'set')
      this.insertCSSStub = sinon.stub(chrome.tabs, 'insertCSS')
    })
    after(function () {
      this.getStorageStub.restore()
      this.setStorageStub.restore()
      this.insertCSSStub.restore()
    })
    beforeEach(function () {
      this.getStorageStub.reset()
      this.setStorageStub.reset()
      this.insertCSSStub.reset()
    })
    it('applies the correct filter', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {
          'brave.com': [filter]
        }
      })
      cosmeticFilterAPI.applySiteFilters('brave.com')
      assert.deepEqual(this.insertCSSStub.getCall(0).args[0], {
        code: `${filter} {display: none;}`,
        runAt: 'document_start'
      })
    })
    it('applies multiple filters correctly', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {
          'brave.com': [filter, filter2]
        }
      })
      cosmeticFilterAPI.applySiteFilters('brave.com')
      assert.deepEqual(this.insertCSSStub.getCall(0).args[0], {
        code: `${filter} {display: none;}`,
        runAt: 'document_start'
      })
      assert.deepEqual(this.insertCSSStub.getCall(1).args[0], {
        code: `${filter2} {display: none;}`,
        runAt: 'document_start'
      })

    })
    // chrome.local.storage.get() always returns an empty object if nothing exists
    it('doesn\'t apply filters if storage for host is implicitly undefined', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {}
      })
      cosmeticFilterAPI.applySiteFilters('brave.com')
      assert.equal(this.insertCSSStub.called, false)
    })
    it('doesn\'t apply filters if storage is explicitly undefined', function () {
      this.getStorageStub.yields({
        cosmeticFilterList: {
          'brave.com': undefined
        }
      })
      cosmeticFilterAPI.applySiteFilters('brave.com')
      assert.equal(this.insertCSSStub.called, false)
    })
  })
})
