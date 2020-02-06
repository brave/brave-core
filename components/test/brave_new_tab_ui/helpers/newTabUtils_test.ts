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
      const index: number = 1337
      // Test via startsWith to avoid calling Date.now() which
      // will often fail all tests
      const assertion: string = newTabUtils.generateGridSiteId(index)
      expect(assertion.startsWith(`topsite-${index}`))
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
  describe('isGridSitePinned', () => {
    it('returns true if site.pinnedIndex is defined', () => {
      const site: NewTab.Site = {
        id: '',
        url: 'https://brave.com',
        title: 'brave',
        favicon: '',
        letter: '',
        pinnedIndex: 1337,
        bookmarkInfo: undefined
      }
      expect(newTabUtils.isGridSitePinned(site)).toBe(true)
    })
    it('returns false if site.pinnedIndex is not defined', () => {
      const site: NewTab.Site = {
        id: '',
        url: 'https://brave.com',
        title: 'brave',
        favicon: '',
        letter: '',
        pinnedIndex: undefined,
        bookmarkInfo: undefined
      }
      expect(newTabUtils.isGridSitePinned(site)).toBe(false)
    })
  })
  describe('isGridSiteBookmarked', () => {
    it('return true if bookmarkInfo is defined', () => {
      const site: NewTab.Site = {
        id: '',
        url: 'https://brave.com',
        title: 'brave',
        favicon: '',
        letter: '',
        pinnedIndex: undefined,
        bookmarkInfo: {
          dateAdded: 1337,
          id: '',
          index: 1337,
          parentId: '',
          title: 'brave',
          url: 'https://brave.com'
        }
      }
      expect(newTabUtils.isGridSiteBookmarked(site.bookmarkInfo)).toBe(true)
    })
    it('returns false if bookmarkInfo is not defined', () => {
      const site: NewTab.Site = {
        id: '',
        url: 'https://brave.com',
        title: 'brave',
        favicon: '',
        letter: '',
        pinnedIndex: undefined,
        bookmarkInfo: undefined
      }
      expect(newTabUtils.isGridSiteBookmarked(site.bookmarkInfo)).toBe(false)
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
        'title', 'url', 'id', 'letter', 'favicon', 'pinnedIndex', 'bookmarkInfo'
      ]
      expect(Object.keys(assertion).every(item => expected.includes(item)))
        .toBe(true)
    })
  })
  describe('getGridSitesWhitelist', () => {
    it('excludes https://chrome.google.com/webstore from list', () => {
      const topSites: chrome.topSites.MostVisitedURL[] = [
        { url: 'https://chrome.google.com/webstore', title: 'store' }
      ]
      expect(newTabUtils.getGridSitesWhitelist(topSites)).toHaveLength(0)
    })
    it('does not exclude an arbritary site from list', () => {
      const topSites: chrome.topSites.MostVisitedURL[] = [
        { url: 'https://tmz.com', title: 'tmz' }
      ]
      expect(newTabUtils.getGridSitesWhitelist(topSites)).toHaveLength(1)
    })
  })
})
