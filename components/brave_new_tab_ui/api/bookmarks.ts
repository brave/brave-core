// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * Obtain the URLs bookmark info
 */
export const fetchBookmarkTreeNode = (
  url: string
): Promise<chrome.bookmarks.BookmarkTreeNode> => {
  return new Promise(resolve => {
    chrome.bookmarks.search(
      url,
      (bookmarkTreeNodes) => {
        resolve(bookmarkTreeNodes[0])
      }
    )
  })
}

/**
 * Iterate over the sites array and obtain all URLs
 * bookmark info
 */
export const fetchAllBookmarkTreeNodes = (
  sites: chrome.topSites.MostVisitedURL[]
): Promise<chrome.bookmarks.BookmarkTreeNode[]> => {
  return Promise
    .all(sites.map(site => fetchBookmarkTreeNode(site.url)))
}

/**
 * Update bookmark info based on user interaction
 */
export const updateBookmarkTreeNode = (site: NewTab.Site) => {
  return new Promise(async resolve => {
    const bookmarkInfo = await fetchBookmarkTreeNode(site.url)
    // Toggle the bookmark state
    if (bookmarkInfo) {
      chrome.bookmarks.remove(bookmarkInfo.id)
    } else {
      chrome.bookmarks.create({ title: site.title, url: site.url })
    }
    resolve(bookmarkInfo)
  })
}
