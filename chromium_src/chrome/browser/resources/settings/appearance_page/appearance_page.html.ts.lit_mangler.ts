// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Removes rows Brave either doesn't want, or already shows elsewhere (its own
// toolbar settings, side panel section, or the Tabs settings page), and
// appends <settings-brave-appearance-toolbar>, which holds the settings that
// used to live inline here (e.g. "Use wide address bar").
mangle((root) => {
  const removeElementWithId = (id: string) => {
    const elem = root.getElementById(id)
    if (!elem) {
      throw new Error(`[Settings] Appearance page: couldn't find #${id}`)
    }
    elem.remove()
  }

  // Home button toggle & its options are shown in
  // <settings-brave-appearance-toolbar> instead. The two `div.hr` separators
  // immediately surrounding it (and its home-button-options block, which
  // sits between them as an inert `${...}` expression, not a DOM sibling)
  // become redundant once it's removed.
  const homeButtonToggle = root.querySelector(
    'settings-toggle-button[pref-key="browser.show_home_button"]')
  if (!homeButtonToggle) {
    throw new Error(
      `[Settings] Appearance page: couldn't find home button toggle`)
  }
  homeButtonToggle.previousElementSibling?.remove()
  homeButtonToggle.nextElementSibling?.remove()
  homeButtonToggle.remove()

  // Note: #colorSchemeModeRow, #defaultFontSize and #pageZoom (below) are
  // hidden via CSS instead of removed here, since SettingsAppearancePageElement
  // (appearance_page.ts) declares them in its `$` interface -- removing them
  // from the template would break that. See the CSS added by the companion
  // chromium_src/.../appearance_page.ts override.

  // Remove the side panel position section entirely; Brave has its own
  // separate side panel settings section.
  const sidePanelPosition = root.getElementById('sidePanelPosition')
  if (!sidePanelPosition) {
    throw new Error(
      `[Settings] Appearance page: couldn't find sidePanelPosition`)
  }
  const sidePanelList = sidePanelPosition.closest('.list-frame')
  if (!sidePanelList) {
    throw new Error(
      `[Settings] Appearance page: couldn't find sidePanelPosition container`)
  }
  // The `div.cr-row` heading ("Side panel position") above the list-frame.
  sidePanelList.previousElementSibling?.remove()
  sidePanelList.remove()

  // showSavedTabGroups and autoPinNewTabGroups are shown in
  // <settings-brave-appearance-toolbar> instead, after the bookmark bar
  // setting.
  removeElementWithId('showSavedTabGroups')
  removeElementWithId('autoPinNewTabGroups')

  removeElementWithId('splitViewDragAndDrop')

  // Note: #customize-fonts-subpage-trigger is hidden via CSS instead of
  // removed here (see the companion appearance_page.ts override), since
  // appearance_page.ts's getFocusConfig()/getAssociatedControlFor() still
  // reference it by selector and would misbehave if it were missing from
  // the DOM entirely.

  // Mac-only: shown on the Content page instead.
  root.querySelector(
    'settings-toggle-button[pref-key="webkit.webprefs.tabs_to_links"]')
    ?.remove()
  root.querySelector(
    'settings-toggle-button[pref-key="browser.confirm_to_quit"]')
    ?.remove()

  const section = root.querySelector('settings-section')
  if (!section) {
    throw new Error(
      `[Settings] Appearance page: couldn't find settings-section`)
  }
  section.insertAdjacentHTML(
    'beforeend',
    '<settings-brave-appearance-toolbar></settings-brave-appearance-toolbar>')
})

// Remove the home page options (radio group for NTP vs custom URL). It's
// only shown when the (now-removed) home button toggle is on, and its
// content is redundant since the toggle controlling it is gone from this
// page.
mangle(
  (element) => { element.textContent = '' },
  (t) => t.text.includes('id="home-button-options"'),
)

// Remove the old-style bookmark bar toggle; Brave's own toolbar settings
// area has the replacement (a dropdown with an NTP-only option).
mangle(
  (element) => { element.textContent = '' },
  (t) => t.text.includes('id="showBookmarksBar"'),
)

// Remove the hover card image/memory-usage toggles; Brave shows the
// equivalent (memory usage only) on the Tabs settings page instead.
mangle(
  (element) => { element.textContent = '' },
  (t) => t.text.includes('id="hoverCardImagesToggle"'),
)
mangle(
  (element) => { element.textContent = '' },
  (t) => t.text.includes('hoverCardMemoryUsageToggle')
      && !t.text.includes('hoverCardImagesToggle'),
)
