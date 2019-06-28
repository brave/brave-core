/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_new_tab_ui/constants/new_tab_types'
import * as actions from '../../../brave_new_tab_ui/actions/new_tab_actions'

describe('newTabActions', () => {
  it('topSitesDataUpdated', () => {
    const topSites: Array<string> = []
    expect(actions.topSitesDataUpdated(topSites)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_TOP_SITES_DATA_UPDATED,
      payload: { topSites }
    })
  })
  it('bookmarkAdded', () => {
    const url: string = 'https://brave.com'
    expect(actions.bookmarkAdded(url)).toEqual({
      meta: undefined,
      type: types.BOOKMARK_ADDED,
      payload: { url }
    })
  })
  it('bookmarkRemoved', () => {
    const url: string = 'https://brave.com'
    expect(actions.bookmarkRemoved(url)).toEqual({
      meta: undefined,
      type: types.BOOKMARK_REMOVED,
      payload: { url }
    })
  })
  it('sitePinned', () => {
    const url: string = 'https://brave.com'
    expect(actions.sitePinned(url)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_SITE_PINNED,
      payload: { url }
    })
  })
  it('siteUnpinned', () => {
    const url: string = 'https://brave.com'
    expect(actions.siteUnpinned(url)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_SITE_UNPINNED,
      payload: { url }
    })
  })
  it('siteIgnored', () => {
    const url: string = 'https://brave.com'
    expect(actions.siteIgnored(url)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_SITE_IGNORED,
      payload: { url }
    })
  })
  it('undoSiteIgnored', () => {
    const url: string = 'https://brave.com'
    expect(actions.undoSiteIgnored(url)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_UNDO_SITE_IGNORED,
      payload: { url }
    })
  })
  it('undoAllSiteIgnored', () => {
    const url: string = 'https://brave.com'
    expect(actions.undoAllSiteIgnored(url)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_UNDO_ALL_SITE_IGNORED,
      payload: { url }
    })
  })
  it('siteDragged', () => {
    const fromUrl: string = 'https://brave.com'
    const toUrl: string = 'https://wikipedia.org'
    const dragRight: boolean = true
    expect(actions.siteDragged(fromUrl, toUrl, dragRight)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_SITE_DRAGGED,
      payload: { fromUrl, toUrl, dragRight }
    })
  })
  it('siteDragEnd', () => {
    const url: string = 'https://brave.com'
    const didDrop: boolean = false
    expect(actions.siteDragEnd(url, didDrop)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_SITE_DRAG_END,
      payload: { url, didDrop }
    })
  })
  it('onHideSiteRemovalNotification', () => {
    expect(actions.onHideSiteRemovalNotification()).toEqual({
      meta: undefined,
      type: types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION
    })
  })
  it('bookmarkInfoAvailable', () => {
    const queryUrl: string = 'https://brave.com'
    const bookmarkTreeNode = {
      dateAdded: 1557899510259,
      id: '7',
      index: 0,
      parentId: '2',
      title: 'Secure, Fast & Private Web Browser with Adblocker | Brave Browser',
      url: 'http://brave.com/'
    }
    expect(actions.bookmarkInfoAvailable(queryUrl, bookmarkTreeNode)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_BOOKMARK_INFO_AVAILABLE,
      payload: { queryUrl, bookmarkTreeNode }
    })
  })
  it('gridSitesUpdated', () => {
    const gridSites: Array<NewTab.Sites> = [
      {
        bookmarked: undefined,
        favicon: 'chrome://favicon/size/64@1x/http://brave.com/',
        index: 0,
        letter: 'B',
        pinned: true,
        thumb: 'chrome://thumb/http://brave.com/',
        title: 'Secure, Fast & Private Web Browser with Adblocker | Brave Browser',
        url: 'http://brave.com/'
      }
    ]
    expect(actions.gridSitesUpdated(gridSites)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_GRID_SITES_UPDATED,
      payload: { gridSites }
    })
  })
  it('statsUpdated', () => {
    expect(actions.statsUpdated()).toEqual({
      meta: undefined,
      type: types.NEW_TAB_STATS_UPDATED
    })
  })
  it('changePrivateSearchEngine', () => {
    const shouldUse: boolean = true
    expect(actions.changePrivateSearchEngine(shouldUse)).toEqual({
      meta: undefined,
      type: types.NEW_TAB_USE_ALTERNATIVE_PRIVATE_SEARCH_ENGINE,
      payload: { shouldUse }
    })
  })
  it('showSettingsMenu', () => {
    expect(actions.showSettingsMenu()).toEqual({
      type: types.NEW_TAB_SHOW_SETTINGS_MENU
    })
  })
  it('closeSettingsMenu', () => {
    expect(actions.showSettingsMenu()).toEqual({
      type: types.NEW_TAB_SHOW_SETTINGS_MENU
    })
  })
})
