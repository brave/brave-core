/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as bookmarksAPI from '../../../../brave_new_tab_ui/api/topSites/bookmarks'

const state = {
  backgroundImage: {},
  bookmarks: {},
  gridSites: [],
  ignoredTopSites: [],
  isIncognito: false,
  isQwant: false,
  isTor: false,
  pinnedTopSites: [],
  showEmptyPage: false,
  showSiteRemovalNotification: false,
  stats: {},
  topSites: [],
  useAlternativePrivateSearchEngine: false
}

describe('new tab bookmarks api tests', () => {
  describe('fetchBookmarkInfo', () => {
    let spy: jest.SpyInstance
    const url = 'https://brave.com'
    beforeEach(() => {
      spy = jest.spyOn(chrome.bookmarks, 'search')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.bookmarks.search', () => {
      bookmarksAPI.fetchBookmarkInfo(url)
      expect(spy).toBeCalled()
    })
  })

  describe('updateBookmarkInfo', () => {
    const url = 'https://brave.com'
    it('bookmarks the url if bookmark has a tree node', () => {
      const updateBookmarkInfo = bookmarksAPI.updateBookmarkInfo(state, url, true)
      const assertion = updateBookmarkInfo.bookmarks
      expect(assertion).toEqual({ 'https://brave.com': true })
    })
    it('sets bookmark to undefined if tree node is not defined', () => {
      const updateBookmarkInfo = bookmarksAPI.updateBookmarkInfo(state, url, false)
      const assertion = updateBookmarkInfo.bookmarks
      expect(assertion).toEqual({ 'https://brave.com': undefined })
    })
  })
})
