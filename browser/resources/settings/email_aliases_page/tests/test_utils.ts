// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'

/**
 * Creates a RegExp from a locale string key.
 * This is useful for matching localized text in tests, as it handles
 * potential whitespace and formatting differences.
 */
export const localeRegex = (key: string) => new RegExp(getLocale(key))

// Helper function to click a Leo button
export const clickLeoButton = (button: Element) => {
  button.shadowRoot?.querySelector('button')?.click()
}
