// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../brave_appearance_page/super_referral.js'
import '../brave_appearance_page/brave_theme.js'

import {html, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'

import {loadTimeData} from '../i18n_setup.js'
import {Router} from '../router.js'

const superReferralStringId = 'superReferralThemeName'

RegisterPolymerTemplateModifications({
  'settings-appearance-page': (templateContent) => {
    const theme = templateContent.getElementById('themeRow')
    if (!theme) {
      console.error(`[Brave Settings Overrides] Couldn't find #themeRow`)
    } else {
      const useDefaultButtonTemplate = templateContent.querySelector(
        'template[is=dom-if][if="[[prefs.extensions.theme.id.value]]"]')
      if (!useDefaultButtonTemplate) {
        console.error(
          '[Brave Settings Overrides] Appearance Page cannot find use default' +
          ' theme button template')
      } else {
        useDefaultButtonTemplate.setAttribute("restamp", "true")
      }
      theme.setAttribute("class", "settings-row hr")
      theme.insertAdjacentHTML(
        'beforebegin',
        getTrustedHTML`
          <settings-brave-appearance-theme prefs="{{prefs}}">
          </settings-brave-appearance-theme>
        `)
    }

    // Super-referral
    // W/o super referral, we don't need to themes link option with themes sub
    // page.
    const hasSuperReferral = (
      loadTimeData.valueExists(superReferralStringId) &&
      loadTimeData.getString(superReferralStringId) !== ''
    )
    if (hasSuperReferral) {
      // Routes
      const r = Router.getInstance().routes_
      if (!r.APPEARANCE) {
        console.error(
          '[Brave Settings Overrides] Routes: could not find APPEARANCE page')
        return
      } else {
        r.THEMES = r.APPEARANCE.createChild('/themes');
        // Hide chromium's theme section. It's replaced with our themes page.
        if (theme) {
          theme.remove()
        }
      }
      // Subpage
      const pages = templateContent.getElementById('pages')
      if (!pages) {
        console.error(
          `[Brave Settings Overrides] Couldn't find appearance_page #pages`)
      } else {
        pages.appendChild(
          html`
            <template is="dom-if" route-path="/themes">
              <settings-subpage
                associated-control="[[$$('#themes-subpage-trigger')]]"
                page-title="${loadTimeData.getString('themes')}">
                <settings-brave-appearance-super-referral prefs="{{prefs}}">
                </settings-brave-appearance-super-referral>
              </settings-subpage>
            </template>
          `)
      }
    }

    // Remove home button toggle & options template as it's moved into
    // address bar sub section
    const homeButtonToggle = templateContent.querySelector(
      '[pref="{{prefs.browser.show_home_button}}"]')
    if (!homeButtonToggle) {
      console.error(
        `[Brave Settings Overrides] Couldn't find home button toggle`)
    } else {
      homeButtonToggle.remove()
    }
    const homeButtonOptionsTemplate = templateContent.querySelector(
        'template[is=dom-if][if="[[prefs.browser.show_home_button.value]]"]')
    if (!homeButtonOptionsTemplate) {
      console.error(
        `[Brave Settings Overrides] Couldn't find home button options template`)
    } else {
      homeButtonOptionsTemplate.remove()
    }

    const bookmarkBarToggle = templateContent.querySelector(
      '[pref="{{prefs.bookmark_bar.show_on_all_tabs}}"]')
    if (!bookmarkBarToggle) {
      console.error(
        `[Brave Settings Overrides] Couldn't find bookmark bar toggle`)
    } else {
      // Remove Chromium bookmark toggle becasue it is replaced by
      // settings-brave-appearance-bookmark-bar
      bookmarkBarToggle.remove()
    }

    // Hide or remove upstream's font options.
    const defaultFontSize = templateContent.querySelector(
      '.cr-row:has(#defaultFontSize)')
    if (!defaultFontSize) {
      console.error(`[Brave Settings Overrides] Couldn't find default font size option`)
    } else {
      // Just hide instead of removing as upstream js refers this.
      defaultFontSize.setAttribute("hidden", "true")
    }
    const customizeFontsSubpageTrigger = templateContent.getElementById('customize-fonts-subpage-trigger')
    if (!customizeFontsSubpageTrigger) {
      console.error(`[Brave Settings Overrides] Couldn't find customize fonts subpage trigger`)
    } else {
      customizeFontsSubpageTrigger.remove()
    }
    const customizeFontsTemplate = templateContent.querySelector(
        'template[is=dom-if][route-path="/fonts"]')
    if (!customizeFontsTemplate) {
      console.error(`[Brave Settings Overrides] Couldn't find customize fonts subpage template`)
    } else {
      customizeFontsTemplate.remove()
    }
    const pageZoom = templateContent.querySelector('.cr-row:has(#pageZoom)')
    if (!pageZoom) {
      console.error(`[Brave Settings Overrides] Couldn't find page zoom`)
    } else {
      pageZoom.remove()
    }

    const hrsToHide = templateContent.querySelectorAll('div.hr:not(#themeRow):not([hidden="[[!pageVisibility.bookmarksBar]]"])');
    // We only want to hide two hrs now from upstream appearance_page.html.
    if (hrsToHide.length !== 2) {
      console.error(`[Brave Settings Overrides] detected more than two hrs to hide`)
    } else {
      for (const hr of hrsToHide) {
        hr.setAttribute("hidden", "true")
      }
    }

    // <if expr="is_macosx">
    const confirmToQuit = templateContent.querySelector(
      '[pref="{{prefs.browser.confirm_to_quit}}"]')
    if (!confirmToQuit) {
      console.error(`[Brave Settings Overrides] Couldn't find confirm to quit`)
    } else {
      confirmToQuit.remove()
    }

    const tabsToLinks = templateContent.querySelector(
      '[pref="{{prefs.webkit.webprefs.tabs_to_links}}"]')
    if (!tabsToLinks) {
      console.error(`[Brave Settings Overrides] Couldn't find tabs to links`)
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
        '[Brave Settings Overrides] Appearance Page cannot find hover card' +
        ' images template with !showHoverCardImagesOption_')
    } else {
      hoverCardImagesTemplateNotShow.remove()
    }
    const hoverCardImagesTemplateShow = templateContent.querySelector(
      'template[is=dom-if][if="[[showHoverCardImagesOption_]]"]')
    if (!hoverCardImagesTemplateShow) {
      console.error(
        '[Brave Settings Overrides] Appearance Page cannot find hover card' +
        ' images template with showHoverCardImagesOption_')
    } else {
      hoverCardImagesTemplateShow.remove()
    }
  },
})
