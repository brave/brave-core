/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StringKey } from '../lib/locale_strings'

const localeStrings: { [K in StringKey]: string }  = {
  backgroundSettingsTitle: 'Background Image',
  braveBackgroundLabel: 'Brave backgrounds',
  customBackgroundLabel: 'Use your own',
  customBackgroundTitle: 'Use your own',
  gradientBackgroundLabel: 'Gradients',
  gradientBackgroundTitle: 'Gradients',
  photoCreditsText: 'Photo by $1',
  randomizeBackgroundLabel: 'Refresh on every new tab',
  settingsTitle: 'Customize Dashboard',
  showBackgroundsLabel: 'Show background images',
  showSponsoredImagesLabel: 'Show Sponsored Images',
  solidBackgroundLabel: 'Solid colors',
  solidBackgroundTitle: 'Solid colors',
  uploadBackgroundLabel: 'Upload from device'
}

export function createLocale() {
  const getString =
    (key: string) => String((localeStrings as any)[key] || 'MISSING')
  return {
    getString,
    async getPluralString (key: string, count: number) {
      return getString(key).replace('#', String(count))
    }
  }
}
