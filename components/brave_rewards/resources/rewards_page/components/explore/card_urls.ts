/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function sanitizeURL(url: string) {
  try {
    return new URL(url).protocol === 'https:' ? url : ''
  } catch {
    return ''
  }
}

export function faviconURL(url: string) {
  return `chrome://favicon2/?size=64&pageUrl=${encodeURIComponent(url)}`
}

export function cardImageURL(url: string) {
  url = sanitizeURL(url)
  try {
    if (/(^|\.)brave(software)?\.com$/i.test(new URL(url).hostname)) {
      return `chrome://image?url=${encodeURIComponent(url)}`
    }
    return ''
  } catch {
    return ''
  }
}
