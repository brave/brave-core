// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const isHttpOrHttps = (url?: string) => {
  if (!url) {
    return false
  }
  return /^https?:/i.test(url)
}

export const getCharForSite = (
  topSite: chrome.topSites.MostVisitedURL
): string => {
  let hostname: string = '?'
  if (!topSite.title) {
    try {
      hostname = new window.URL(topSite.url || '').hostname
    // tslint:disable-next-line: no-empty
    } catch (e) {}
  }
  const name: string = topSite.title || hostname
  return name.charAt(0).toUpperCase()
}

export const generateGridSiteId = (currentIndex: number): string => {
  return `topsite-${currentIndex}-${Date.now()}`
}

export const generateGridSiteFavicon = (url: string): string => {
  return `chrome://favicon/size/64@1x/${url}`
}

export const isGridSitePinned = (
  gridSite: NewTab.Site
): boolean => {
  return gridSite.pinnedIndex !== undefined
}

export const isGridSiteBookmarked = (
  bookmarkInfo: chrome.bookmarks.BookmarkTreeNode | undefined
): boolean => {
  return bookmarkInfo !== undefined
}

export const isExistingGridSite = (
  sitesData: NewTab.Site[],
  topOrGridSite: chrome.topSites.MostVisitedURL | NewTab.Site
): boolean => {
  return sitesData.some(site => site.url === topOrGridSite.url)
}

export const generateGridSiteProperties = (
  index: number,
  topSite: chrome.topSites.MostVisitedURL
): NewTab.Site => {
  return {
    ...topSite,
    id: generateGridSiteId(index),
    letter: getCharForSite(topSite),
    favicon: generateGridSiteFavicon(topSite.url),
    pinnedIndex: undefined,
    bookmarkInfo: undefined
  }
}

export const getGridSitesWhitelist = (
  topSites: chrome.topSites.MostVisitedURL[]
): chrome.topSites.MostVisitedURL[] => {
  const defaultChromeWebStoreUrl: string = 'https://chrome.google.com/webstore'
  const filteredGridSites: chrome.topSites.MostVisitedURL[] = topSites
    .filter(site => {
      // See https://github.com/brave/brave-browser/issues/5376
      return !site.url.startsWith(defaultChromeWebStoreUrl)
    })
  return filteredGridSites
}
