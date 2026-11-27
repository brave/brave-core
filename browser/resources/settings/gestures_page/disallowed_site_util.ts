/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {toUrl} from './url_scheme_util.js'
import type {DisallowedSiteRow} from './gesture_rows.js'

// Reduces a URL to its origin, stored as the pref dict key and shown as-is
// in the list. A non-http(s) scheme names one page, not a site, so it stays.
export function toDisallowedSiteOrigin(input: string): string {
  const trimmedInput = input.trim()
  const url = toUrl(trimmedInput)
  if (!url) {
    return trimmedInput.toLowerCase()
  }
  const origin = url.protocol === 'http:' || url.protocol === 'https:'
    ? url.hostname
    : `${url.protocol}//${url.hostname}`
  return origin.toLowerCase()
}

export function disallowedSiteRowsFromPref(
  disallowedSites: Record<string, boolean> | undefined):
  DisallowedSiteRow[] {
  return Object.keys(disallowedSites ?? {}).map((origin) => ({ origin }))
}

export function addDisallowedSiteDialogTitle(
  editingDisallowedSiteOrigin: string | null,
  i18n: (key: string) => string): string {
  return editingDisallowedSiteOrigin !== null
    ? i18n('braveGesturesEditSite')
    : i18n('braveGesturesAddSite')
}
