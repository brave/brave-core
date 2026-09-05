// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// The card's own label is always the "site search" heading, regardless of
// the searchSettingsUpdate flag that governs the page-level title (kept as
// upstream computes it, since it's also used for the page's breadcrumb).
mangle((root) => {
  const title = root.querySelector('.default-search-engine')
  if (!title) {
    throw new Error(
      `[Settings] Search page: couldn't find .default-search-engine`)
  }
  const textNode = title.firstChild
  if (!textNode || textNode.nodeType !== textNode.TEXT_NODE) {
    throw new Error(
      `[Settings] Search page: couldn't find search page title text node`)
  }
  textNode.textContent = '$i18n{normalSearchEnginesSiteSearchEngineHeading}'
}, (t) => t.text.includes('default-search-engine'))

// Swap in our version of the search engine list dialog, which adds the
// Brave search engine and the guest-mode "save choice" checkbox. It takes
// the same `searchEngines`/`close`/`search-engine-changed` bindings as
// upstream's, so only the tag name needs to change.
mangle((root) => {
  const dialog = root.querySelector('settings-search-engine-list-dialog')
  if (!dialog) {
    throw new Error(
      `[Settings] Search page: couldn't find search engine list dialog`)
  }
  dialog.insertAdjacentHTML(
    'beforebegin',
    '<settings-normal-search-engine-list-dialog' +
    ' .searchEngines="${this.searchEngines_}"' +
    ' @close="${this.onSearchEngineListDialogClose_}"' +
    ' @search-engine-changed="${this.onSearchEngineChanged_}">' +
    '</settings-normal-search-engine-list-dialog>')
  dialog.remove()
}, (t) => t.text.includes('settings-search-engine-list-dialog'))

// Add our private-search-engine card above the "Manage search engines"
// link row. `.prefs` is forwarded from the settings-prefs singleton by the
// companion search_page.ts override, since this Lit page has no `prefs`
// property of its own to bind. Gated on bravePrefs_ being loaded so the element
// never connects with an empty/placeholder `prefs`.
mangle((root) => {
  const enginesSubpageTrigger = root.getElementById('enginesSubpageTrigger')
  if (!enginesSubpageTrigger) {
    throw new Error(
      `[Settings] Search page: couldn't find #enginesSubpageTrigger`)
  }
  enginesSubpageTrigger.insertAdjacentHTML(
    'beforebegin',
    '${this.bravePrefs_ ? html`' +
    '<settings-brave-search-page .prefs="${this.bravePrefs_}">' +
    '</settings-brave-search-page>' +
    '` : \'\'}')
}, (t) => t.text.includes('enginesSubpageTrigger'))
