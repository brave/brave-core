/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../brave_appearance_page/toolbar.js'

import {
  html,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'

import { loadTimeData } from '../i18n_setup.js'


RegisterPolymerTemplateModifications({
  'settings-appearance-page': (templateContent) => {
    // Remove home button toggle & options template as it's moved into
    // address bar sub section
    const homeButtonToggle = templateContent.querySelector(
      '[pref="{{prefs.browser.show_home_button}}"]')
    if (!homeButtonToggle) {
      console.error(
        `[Settings] Couldn't find home button toggle`)
    } else {
      homeButtonToggle.remove()
    }
    const homeButtonOptionsTemplate = templateContent.querySelector(
      'template[is=dom-if][if="[[prefs.browser.show_home_button.value]]"]')
    if (!homeButtonOptionsTemplate) {
      console.error(
        `[Settings] Couldn't find home button options template`)
    } else {
      homeButtonOptionsTemplate.remove()
    }

    const bookmarkBarToggle = templateContent.querySelector(
      '[pref="{{prefs.bookmark_bar.show_on_all_tabs}}"]')
    if (!bookmarkBarToggle) {
      console.error(
        `[Settings] Couldn't find bookmark bar toggle`)
    } else {
      // Remove Chromium bookmark toggle becasue it is replaced by
      // settings-brave-appearance-bookmark-bar
      bookmarkBarToggle.remove()
    }

    // Hide or remove upstream's font options.
    const defaultFontSize = templateContent.querySelector(
      '.cr-row:has(#defaultFontSize)')
    if (!defaultFontSize) {
      console.error(`[Settings] Couldn't find default font size option`)
    } else {
      // Just hide instead of removing as upstream js refers this.
      defaultFontSize.setAttribute('hidden', 'true')
    }
    const customizeFontsSubpageTrigger = templateContent.getElementById('customize-fonts-subpage-trigger')
    if (!customizeFontsSubpageTrigger) {
      console.error(`[Settings] Couldn't find customize fonts subpage trigger`)
    } else {
      customizeFontsSubpageTrigger.remove()
    }
    const pageZoom = templateContent.querySelector('.cr-row:has(#pageZoom)')
    if (!pageZoom) {
      console.error(`[Settings] Couldn't find page zoom`)
    } else {
      pageZoom.remove()
    }

    const hrsToHide = templateContent.querySelectorAll(
      'div.hr:not(#themeRow):not([hidden="[[!pageVisibility.bookmarksBar]]"])')
    // We only want to hide two hrs now from upstream appearance_page.html.
    if (hrsToHide.length !== 2) {
      console.error(`[Settings] detected more than two hrs to hide`)
    } else {
      for (const hr of hrsToHide) {
        hr.setAttribute('hidden', 'true')
      }
    }

    // <if expr="is_macosx">
    const confirmToQuit = templateContent.querySelector(
      '[pref="{{prefs.browser.confirm_to_quit}}"]')
    if (!confirmToQuit) {
      console.error(`[Settings] Couldn't find confirm to quit`)
    } else {
      confirmToQuit.remove()
    }

    const tabsToLinks = templateContent.querySelector(
      '[pref="{{prefs.webkit.webprefs.tabs_to_links}}"]')
    if (!tabsToLinks) {
      console.error(`[Settings] Couldn't find tabs to links`)
    } else {
      tabsToLinks.remove()
    }
    // </if>

    // Remove show images on tab hover toggle and tab memory usage toggle as we
    // already have settings for these in the Tabs settings.
    const hoverCardImagesTemplateNotShow = templateContent.querySelector(
      'template[is=dom-if][if="[[!showHoverCardImagesOption_]]"]')
    if (!hoverCardImagesTemplateNotShow) {
      console.error(
        '[Settings] Appearance Page cannot find hover card' +
        ' images template with !showHoverCardImagesOption_')
    } else {
      hoverCardImagesTemplateNotShow.remove()
    }
    const hoverCardImagesTemplateShow = templateContent.querySelector(
      'template[is=dom-if][if="[[showHoverCardImagesOption_]]"]')
    if (!hoverCardImagesTemplateShow) {
      console.error(
        '[Settings] Appearance Page cannot find hover card' +
        ' images template with showHoverCardImagesOption_')
    } else {
      hoverCardImagesTemplateShow.remove()
    }
    const colorSchemeModeRow = templateContent.getElementById(
      'colorSchemeModeRow')
    if (!colorSchemeModeRow) {
      console.error(`[Settings] Couldn't find colorSchemeModeRow`)
    } else {
      colorSchemeModeRow.remove()
    }

    // Remove upstream side panel header and dropdown from Appearance as we
    // have our own separate side panel section
    const sidePanelPosition =
      templateContent.getElementById('sidePanelPosition')
    if (!sidePanelPosition) {
      console.error(`[Settings] Couldn't find sidePanelPosition`)
    } else if (sidePanelPosition.parentNode) {
      sidePanelPosition.parentNode.setAttribute('hidden', 'true')
    }

    const section = templateContent.querySelector('settings-section')
    if (!section) {
      console.error(`[Settings] Couldn't find settings-section`)
      return
    }
    // Append toolbar settings to the appearance section.
    section.appendChild(html`
      <settings-brave-appearance-toolbar
        prefs="{{prefs}}">
      </settings-brave-appearance-toolbar>
    `)

    // Remove a couple of items to from appearance page as we have them in
    // <settings-brave-appearance-toolbar>
    // - showSavedTabGroups and autoPinNewTabGroups should be after bookmark bar setting
    const removeElementWithId = (id: string) => {
      const elem = templateContent.querySelector(`#${id}`)
      if (!elem) {
        console.error(`[Settings] Couldn't find element with id: ${id}`)
      } else {
        elem.remove()
      }
    }
    removeElementWithId('showSavedTabGroups')
    removeElementWithId('autoPinNewTabGroups')

    // Remove "Allow split view drag-and-drop on left or right edge of window"
    // toggle as we show it on the Content page.
    removeElementWithId('splitViewDragAndDrop')
  }
})
