/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const allowedProtocols = new Set(['https:', 'chrome:'])

export function sanitizeExternalURL(urlString: string) {
  let url: URL | null = null
  try {
    url = new URL(urlString)
  } catch {
    return ''
  }
  if (!allowedProtocols.has(url.protocol)) {
    return ''
  }
  return url.toString()
}
