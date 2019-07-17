/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 // Constants
import { types } from '../../../brave_new_tab_ui/constants/new_tab_types'

// Reducer
import newTabReducer from '../../../brave_new_tab_ui/reducers/new_tab_reducer'

// State
import { newTabInitialState } from '../../testData'

// API
import * as gridAPI from '../../../brave_new_tab_ui/api/topSites/grid'
import * as bookmarksAPI from '../../../brave_new_tab_ui/api/topSites/bookmarks'
import * as dndAPI from '../../../brave_new_tab_ui/api/topSites/dnd'
import * as storage from '../../../brave_new_tab_ui/storage'

const initialState = newTabInitialState.newTabData

describe('newTabReducer', () => {
  const url: string = 'http://brave.com/'
  const topSites: Partial<NewTab.Sites> = [{ url }]
  const pinnedTopSites: Partial<NewTab.Sites> = topSites
  const ignoredTopSites: Partial<NewTab.Sites> = [{ url: 'https://github.com' }]
  const bookmarks: Partial<NewTab.Bookmark> = { [url]: { id: 'bookmark_id' } }
  const fakeState = {
    ...initialState,
    topSites,
    bookmarks,
    pinnedTopSites,
    ignoredTopSites
  }

  describe('initial state', () => {
    it('loads initial data', () => {
      const expectedState = storage.load()
      const returnedState = newTabReducer(undefined, {})
      expect(returnedState).toEqual(expectedState)
    })
  })
  describe('BOOKMARK_ADDED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.bookmarks, 'create')
    })
    afterEach(() => {
      spy.mockRestore()
    })

    it('calls chrome.bookmarks.create if topSites url match payload url', () => {
      newTabReducer(fakeState, {
        type: types.BOOKMARK_ADDED,
        payload: { url }
      })
      expect(spy).toBeCalled()
    })
    it('does not call chrome.bookmarks.create if url does not match', () => {
      newTabReducer(fakeState, {
        type: types.BOOKMARK_ADDED,
        payload: { url: 'https://very-different-website-domain.com' }
      })
      expect(spy).not.toBeCalled()
    })
  })
  describe('BOOKMARK_REMOVED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.bookmarks, 'remove')
    })
    afterEach(() => {
      spy.mockRestore()
    })

    it('calls chrome.bookmarks.remove if bookmarkInfo exists', () => {
      newTabReducer(fakeState, {
        type: types.BOOKMARK_REMOVED,
        payload: { url }
      })
      expect(spy).toBeCalled()
    })
    it('does not call chrome.bookmarks.remove if bookmarkInfo is undefined', () => {
      const newTabInitialStateWithoutBookmarks = { ...initialState, bookmarks: {} }
      newTabReducer(newTabInitialStateWithoutBookmarks, {
        type: types.BOOKMARK_REMOVED,
        payload: { url }
      })
      expect(spy).not.toBeCalled()
    })
  })
  describe('NEW_TAB_SITE_PINNED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(gridAPI, 'calculateGridSites')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls gridAPI.calculateGridSites', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_SITE_PINNED,
        payload: { url }
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_SITE_UNPINNED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(gridAPI, 'calculateGridSites')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls gridAPI.calculateGridSites', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_SITE_UNPINNED,
        payload: { url }
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_SITE_IGNORED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(gridAPI, 'calculateGridSites')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls gridAPI.calculateGridSites', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_SITE_IGNORED,
        payload: { url }
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_UNDO_SITE_IGNORED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(gridAPI, 'calculateGridSites')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls gridAPI.calculateGridSites', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_UNDO_SITE_IGNORED,
        payload: { url }
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_UNDO_ALL_SITE_IGNORED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(gridAPI, 'calculateGridSites')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls gridAPI.calculateGridSites', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_UNDO_ALL_SITE_IGNORED
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION', () => {
    it('set showSiteRemovalNotification to false', () => {
      const assertion = newTabReducer(fakeState, {
        type: types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION
      })
      expect(assertion).toEqual({
        ...fakeState,
        showSiteRemovalNotification: false
      })
    })
  })
  describe('NEW_TAB_SITE_DRAGGED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(dndAPI, 'onDraggedSite')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls dndAPI.onDraggedSite', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_SITE_DRAGGED,
        payload: {
          fromUrl: 'https://brave.com',
          toUrl: 'https://github.com'
        }
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_SITE_DRAG_END', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(dndAPI, 'onDragEnd')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls dndAPI.onDragEnd', () => {
      newTabReducer(fakeState, {
        type: types.NEW_TAB_SITE_DRAG_END
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_BOOKMARK_INFO_AVAILABLE', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(bookmarksAPI, 'updateBookmarkInfo')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls bookmarksAPI.updateBookmarkInfo', () => {
      const queryUrl: string = 'https://brave.com'
      const bookmarkTreeNode = {
        dateAdded: 1557899510259,
        id: '7',
        index: 0,
        parentId: '2',
        title: 'Secure, Fast & Private Web Browser with Adblocker | Brave Browser',
        url: 'http://brave.com/'
      }
      newTabReducer(fakeState, {
        type: types.NEW_TAB_BOOKMARK_INFO_AVAILABLE,
        payload: {
          queryUrl,
          bookmarkTreeNode
        }
      })
      expect(spy).toBeCalled()
    })
  })
  describe('NEW_TAB_GRID_SITES_UPDATED', () => {
    it('sets gridSites into gridSites state', () => {
      const url: string = 'http://brave.com/'
      const gridSites: Partial<NewTab.Sites> = [{ url }]
      const assertion = newTabReducer(fakeState, {
        type: types.NEW_TAB_GRID_SITES_UPDATED,
        payload: { gridSites }
      })
      expect(assertion).toEqual({
        ...fakeState,
        gridSites: [ { url: 'http://brave.com/' } ]
      })
    })
  })

  describe('NEW_TAB_SHOW_SETTINGS_MENU', () => {
    it('should set show settings to true', () => {
      const mockState = { ...fakeState, showSettings: false }
      const assertion = newTabReducer(mockState, {
        type: types.NEW_TAB_SHOW_SETTINGS_MENU
      })
      const expected = {
        ...mockState,
        showSettings: true
      }
      expect(assertion).toEqual(expected)
    })
  })

  describe('NEW_TAB_CLOSE_SETTINGS_MENU', () => {
    it('should set show settings to false', () => {
      const mockState = { ...fakeState, showSettings: true }
      const assertion = newTabReducer(mockState, {
        type: types.NEW_TAB_CLOSE_SETTINGS_MENU
      })
      const expected = {
        ...mockState,
        showSettings: false
      }
      expect(assertion).toEqual(expected)
    })
  })
})
