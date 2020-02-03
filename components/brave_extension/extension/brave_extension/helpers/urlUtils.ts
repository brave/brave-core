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
 * Returns true for browser built in pages
 * @param {string} url - The URL to check
 */
export const isBrowserUrl = (url: string) => {
  return /^(brave:|chrome:|about:|devtools:)/.test(url)
}

/**
 * Get the URL origin via Web API
 * @param {string} url - The URL to get the origin from
 */
export const getOrigin = (url: string) => {
  // for URLs such as blob:// and data:// that doesn't have a
  // valid origin, return the full url.
  if (!isHttpOrHttps(url)) {
    return url
  }
  return new window.URL(url).origin
}

/**
 * Get the URL hostname via Web API
 * @param {string} url - The URL to get the origin from
 */
export const getHostname = (url: string) => {
  // for URLs such as blob:// and data:// that doesn't have a
  // valid origin, return the full url.
  if (!isHttpOrHttps(url)) {
    return url
  }
  return new window.URL(url).hostname
}

/**
 * Strip http/https protocol
 * @param {string} url - The URL to strip the protocol from
 */
export const stripProtocolFromUrl = (url: string) => url.replace(/(^\w+:|^)\/\//, '')
