// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as newTabUtils from '../../../brave_new_tab_ui/helpers/newTabUtils'

describe('new tab util files tests', () => {
  describe('isHttpOrHttps', () => {
    it('matches http when defined as a protocol type', () => {
      const url = 'http://some-boring-unsafe-website.com'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(true)
    })
    it('matches https when defined as a protocol type', () => {
      const url = 'https://some-nice-safe-website.com'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(true)
    })
    it('does not match http when defined as an origin', () => {
      const url = 'file://http.some-website-tricking-you.com'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(false)
    })
    it('does not match https when defined as an origin', () => {
      const url = 'file://https.some-website-tricking-you.com'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(false)
    })
    it('does not match other protocol', () => {
      const url = 'ftp://some-old-website.com'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(false)
    })
    it('does not match when url is not defined', () => {
      const url = undefined
      expect(newTabUtils.isHttpOrHttps(url)).toBe(false)
    })
    it('matches uppercase http', () => {
      const url = 'HTTP://SCREAMING-SAFE-WEBSITE.COM'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(true)
    })
    it('matches uppercase https', () => {
      const url = 'HTTP://SCREAMING-UNSAFE-WEBSITE.COM'
      expect(newTabUtils.isHttpOrHttps(url)).toBe(true)
    })
  })

  describe('getCharForSite', () => {
    it('returns the first letter of a given URL without subdomains', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com',
        title: 'brave'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with subdomains', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://awesome-sub-domain.brave.com',
        title: 'awesome'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('A')
    })
    it('returns the first letter of a given URL with ports', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com:9999',
        title: 'brave'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with paths', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com/hello-test/', title: 'brave'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with queries', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com/?randomId',
        title: 'brave'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with parameters', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com/?randomId=123123123',
        title: 'brave'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('B')
    })
    it('returns the first letter of a given URL with fragments', () => {
      const url: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com/?randomId=123123123&hl=en#00h00m10s',
        title: 'brave'
      }
      expect(newTabUtils.getCharForSite(url)).toBe('B')
    })
  })

  describe('generateGridSiteId', () => {
    it('returns the id with the correct structure', () => {
      // Test via startsWith to avoid calling Date.now() which
      // will often fail all tests
      const assertion: string = newTabUtils.generateGridSiteId()
      expect(assertion.startsWith(`topsite-`))
        .toBe(true)
    })
  })

  describe('generateGridSiteFavicon', () => {
    it('returns the correct schema for favicons', () => {
      const url: string = 'https://brave.com'
      expect(newTabUtils.generateGridSiteFavicon(url))
        .toBe(`chrome://favicon/size/64@1x/${url}`)
    })
  })
  describe('isExistingGridSite', () => {
    const sites: chrome.topSites.MostVisitedURL[] = [
      { url: 'https://brave.com', title: 'brave' },
      { url: 'https://twitter.com/brave', title: 'brave twitter' }
    ]

    it('returns true if site exists in the list', () => {
      expect(newTabUtils.isExistingGridSite(sites, sites[0])).toBe(true)
    })
    it('returns false if site does not exist in the list', () => {
      const newUrl: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com/about',
        title: 'about brave'
      }
      expect(newTabUtils.isExistingGridSite(sites, newUrl)).toBe(false)
    })
  })
  describe('generateGridSiteProperties', () => {
    it('generates grid sites data from top chromium sites api', () => {
      const newUrl: chrome.topSites.MostVisitedURL = {
        url: 'https://brave.com',
        title: 'brave'
      }
      const assertion = newTabUtils.generateGridSiteProperties(1337, newUrl)
      const expected = [
        'title', 'url', 'id', 'letter', 'favicon', 'pinnedIndex', 'defaultSRTopSite'
      ]
      expect(Object.keys(assertion).every(item => expected.includes(item)))
        .toBe(true)
    })
    describe('generateGridSitesFromLegacyEntries', () => {
      const legacyUrlList: NewTab.LegacySite = [{
        index: 1337,
        url: 'https://brave.com',
        title: 'brave',
        favicon: '',
        letter: 'b',
        thumb: '',
        themeColor: '',
        computedThemeColor: '',
        pinned: undefined
      }]

      it('exclude all old properties of a top site', () => {
        const assertion = newTabUtils.generateGridSitesFromLegacyEntries(legacyUrlList)
        expect(assertion[0]).not.toHaveProperty('index')
        expect(assertion[0]).not.toHaveProperty('thumb')
        expect(assertion[0]).not.toHaveProperty('themeColor')
        expect(assertion[0]).not.toHaveProperty('computedThemeColor')
        expect(assertion[0]).not.toHaveProperty('pinned')
        expect(assertion[0]).not.toHaveProperty('bookmarked')
      })
      it('include all new properties of a top site', () => {
        const assertion = newTabUtils.generateGridSitesFromLegacyEntries(legacyUrlList)
        expect(assertion[0]).toHaveProperty('id')
        expect(assertion[0]).toHaveProperty('pinnedIndex')
      })
      it('set pinnedIndex to be the same as the top site index', () => {
        const assertion = newTabUtils.generateGridSitesFromLegacyEntries(legacyUrlList)
        expect(assertion[0].pinnedIndex).toBe(1337)
      })
    })
  })
  describe('getTopSitesWhitelist', () => {
    it('excludes https://chrome.google.com/webstore from list', () => {
      const topSites: chrome.topSites.MostVisitedURL[] = [
        { url: 'https://chrome.google.com/webstore', title: 'store' }
      ]
      expect(newTabUtils.getTopSitesWhitelist(topSites)).toHaveLength(0)
    })
    it('does not exclude an arbritary site from list', () => {
      const topSites: chrome.topSites.MostVisitedURL[] = [
        { url: 'https://tmz.com', title: 'tmz' }
      ]
      expect(newTabUtils.getTopSitesWhitelist(topSites)).toHaveLength(1)
    })
  })
  describe('filterFromExcludedSites', () => {
    const sitesData: NewTab.Site = [{
      id: '',
      url: 'https://brave.com',
      title: 'brave',
      favicon: '',
      letter: '',
      pinnedIndex: undefined
    }]
    it('filter sites already included in the sites list', () => {
      const removedSitesData = [ ...sitesData ]
      const assertion = newTabUtils.filterFromExcludedSites(sitesData, removedSitesData)
      expect(assertion).toHaveLength(0)
    })
    it('does filter sites not included in the sites list', () => {
      const removedSitesData = [{
        id: '',
        url: 'https://new_site.com',
        title: 'new_site',
        favicon: '',
        letter: '',
        pinnedIndex: undefined
      }]
      const assertion = newTabUtils.filterFromExcludedSites(sitesData, removedSitesData)
      expect(assertion).toHaveLength(1)
    })
  })
  describe('filterDuplicatedSitesbyIndexOrUrl', () => {
    const sitesData: NewTab.Site[] = [{
      id: 'topsite-000',
      url: 'https://brave.com',
      title: 'brave',
      favicon: '',
      letter: '',
      pinnedIndex: undefined
    }]
    it('filter sites already included in the sites list', () => {
      const duplicatedSitesData = [ ...sitesData, ...sitesData ]
      const assertion = newTabUtils.filterDuplicatedSitesbyIndexOrUrl(duplicatedSitesData)
      expect(assertion).toHaveLength(1)
    })
    it('does not filter sites not included in the sites list', () => {
      const otherSitesData: NewTab.Site[] = [{
        id: 'topsite-111',
        url: 'https://new_site.com',
        title: 'new_site',
        favicon: '',
        letter: '',
        pinnedIndex: undefined
      }]
      const newSitesData: NewTab.Site[] = [ ...sitesData, ...otherSitesData ]
      const assertion = newTabUtils.filterDuplicatedSitesbyIndexOrUrl(newSitesData)
      expect(assertion).toHaveLength(2)
    })
  })
})
