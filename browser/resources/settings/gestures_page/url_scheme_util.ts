/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Assumes https for a scheme-less entry, matching omnibox behavior.
// `brave://` is left untouched, since it is already a valid destination.
export function withUrlScheme(url: string): string {
  const lowercase = url.toLowerCase()
  const hasScheme = lowercase.startsWith('http://') ||
    lowercase.startsWith('https://') || lowercase.startsWith('brave://')
  return url && !hasScheme ? `https://${url}` : url
}

// Parses `input` as-is; a scheme-less host fails that and is retried with
// `https://` prepended.
export function toUrl(input: string): URL | null {
  try {
    return new URL(input)
  } catch {
    try {
      return new URL(`https://${input}`)
    } catch {
      return null
    }
  }
}
