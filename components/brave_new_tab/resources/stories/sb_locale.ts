/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StringKey } from '../lib/locale_strings'

const localeStrings: { [K in StringKey]: string }  = {
  backgroundSettingsTitle: 'Background Image',
  braveBackgroundLabel: 'Brave backgrounds',
  customBackgroundLabel: 'Use your own',
  customBackgroundTitle: 'Use your own',
  customizeSearchEnginesLink: 'Customize available engines',
  enabledSearchEnginesLabel: 'Enabled search engines',
  gradientBackgroundLabel: 'Gradients',
  gradientBackgroundTitle: 'Gradients',
  photoCreditsText: 'Photo by $1',
  randomizeBackgroundLabel: 'Refresh on every new tab',
  searchAskLeoDescription: 'Ask Leo',
  searchBoxPlaceholderText: 'Search the web',
  searchBoxPlaceholderTextBrave: 'Ask Brave Search',
  searchCustomizeEngineListText: 'Customize list',
  searchSettingsTitle: 'Search',
  searchSuggestionsDismissButtonLabel: 'No thanks',
  searchSuggestionsEnableButtonLabel: 'Enable',
  searchSuggestionsPromptText: 'When you search, what you type will be sent to your search engine for better suggestions.',
  searchSuggestionsPromptTitle: 'Enable search suggestions?',
  settingsTitle: 'Customize',
  showBackgroundsLabel: 'Show background images',
  showSearchBoxLabel: 'Show search widget in new tabs',
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
