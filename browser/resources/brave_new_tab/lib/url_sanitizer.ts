/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function sanitizeExternalURL(urlString: string) {
  let url: URL | null = null
  try {
    url = new URL(urlString)
  } catch {
    return ''
  }
  if (url.protocol !== 'https:') {
    return ''
  }
  return url.toString()
}
