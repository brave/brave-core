/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StringKey } from '../lib/locale_strings'

const localeStrings: { [K in StringKey]: string }  = {
  addTopSiteLabel: 'Add site',
  addTopSiteTitle: 'Add site',
  backgroundSettingsTitle: 'Background Image',
  braveBackgroundLabel: 'Brave backgrounds',
  cancelButtonLabel: 'Cancel',
  clockFormatLabel: 'Format',
  clockFormatOption12HourText: '12-hour clock',
  clockFormatOption24HourText: '24-hour clock',
  clockFormatOptionAutomaticText: 'Automatic ($1)',
  clockSettingsTitle: 'Clock',
  customBackgroundLabel: 'Use your own',
  customBackgroundTitle: 'Use your own',
  customizeSearchEnginesLink: 'Customize available engines',
  editTopSiteLabel: 'Edit site',
  editTopSiteTitle: 'Edit site',
  enabledSearchEnginesLabel: 'Enabled search engines',
  gradientBackgroundLabel: 'Gradients',
  gradientBackgroundTitle: 'Gradients',
  hideTopSitesLabel: 'Hide top sites',
  photoCreditsText: 'Photo by $1',
  randomizeBackgroundLabel: 'Refresh on every new tab',
  removeTopSiteLabel: 'Remove',
  saveChangesButtonLabel: 'Save changes',
  searchAskLeoDescription: 'Ask Leo',
  searchBoxPlaceholderText: 'Search the web',
  searchBoxPlaceholderTextBrave: 'Ask Brave Search',
  searchCustomizeEngineListText: 'Customize list',
  searchSettingsTitle: 'Search',
  searchSuggestionsDismissButtonLabel: 'No thanks',
  searchSuggestionsEnableButtonLabel: 'Enable',
  searchSuggestionsPromptText: 'When you search, what you type will be sent to your search engine for better suggestions.',
  searchSuggestionsPromptTitle: 'Enable search suggestions?',
  settingsTitle: 'Customize Dashboard',
  showBackgroundsLabel: 'Show background images',
  showClockLabel: 'Show clock',
  showSearchBoxLabel: 'Show search widget in new tabs',
  showSponsoredImagesLabel: 'Show Sponsored Images',
  showTopSitesLabel: 'Show top sites',
  solidBackgroundLabel: 'Solid colors',
  solidBackgroundTitle: 'Solid colors',
  topSiteRemovedText: 'Top site removed',
  topSiteRemovedTitle: 'Removed',
  topSitesCustomOptionText: 'Top sites are curated by you',
  topSitesCustomOptionTitle: 'Favorites',
  topSitesMostVisitedOptionText: 'Top sites are suggested based on websites you visit often.',
  topSitesMostVisitedOptionTitle: 'Frequently Visited',
  topSitesSettingsTitle: 'Top Sites',
  topSitesShowCustomLabel: 'Show favorites',
  topSitesShowMostVisitedLabel: 'Show frequently visited',
  topSitesTitleLabel: 'Name',
  topSitesURLLabel: 'URL',
  undoButtonLabel: 'Undo',
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
