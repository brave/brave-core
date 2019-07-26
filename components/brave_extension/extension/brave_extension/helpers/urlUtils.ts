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
 * Get the URL origin via Web API
 * @param {string} url - The URL to get the origin from
 */
export const getOrigin = (url: string) => new window.URL(url).origin

/**
 * Get the URL hostname via Web API
 * @param {string} url - The URL to get the origin from
 */
export const getHostname = (url: string) => new window.URL(url).hostname

/**
 * Strip http/https protocol
 * @param {string} url - The URL to strip the protocol from
 */
export const stripProtocolFromUrl = (url: string) => url.replace(/(^\w+:|^)\/\//, '')
