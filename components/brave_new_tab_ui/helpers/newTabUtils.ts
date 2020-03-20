/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const isHttpOrHttps = (url?: string) => {
  if (!url) {
    return false
  }
  return /^https?:/i.test(url)
}

/**
 * Obtains a letter / char that represents the current site
 * @param site - The site requested from the top site's list
 */
export const getCharForSite = (site: NewTab.Site) => {
  let name
  if (!site.title) {
    try {
      name = new window.URL(site.url || '').hostname
    } catch (e) {
      console.warn('getCharForSite', { url: site.url || '' })
    }
  }
  name = site.title || name || '?'
  return name.charAt(0).toUpperCase()
}
