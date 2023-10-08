// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../brave_appearance_page/super_referral.js'
import '../brave_appearance_page/brave_theme.js'
import '../brave_appearance_page/toolbar.js'
import '../brave_appearance_page/bookmark_bar.js'

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

    // Toolbar prefs
    const bookmarkBarToggle = templateContent.querySelector(
      '[pref="{{prefs.bookmark_bar.show_on_all_tabs}}"]')
    if (!bookmarkBarToggle) {
      console.error(
        `[Brave Settings Overrides] Couldn't find bookmark bar toggle`)
    } else {
      bookmarkBarToggle.insertAdjacentHTML(
        'beforebegin',
        getTrustedHTML`
          <settings-brave-appearance-bookmark-bar prefs="{{prefs}}">
          </settings-brave-appearance-bookmark-bar>
        `)

      bookmarkBarToggle.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`
          <settings-brave-appearance-toolbar prefs="{{prefs}}">
          </settings-brave-appearance-toolbar>
        `)
      // Remove Chromium bookmark toggle becasue it is replaced by
      // settings-brave-appearance-bookmark-bar
      bookmarkBarToggle.remove()
    }

    // Speedreader and MRU
    const fontsRow =
      templateContent.getElementById('customize-fonts-subpage-trigger')
    if (!fontsRow || !fontsRow.parentNode) {
      console.error(`[Brave Settings Overrides] Couldn't find fonts row by id`)
    } else {
      const isSpeedreaderEnabled =
        loadTimeData.getBoolean('isSpeedreaderFeatureEnabled')
      if (isSpeedreaderEnabled) {
        fontsRow.parentNode.appendChild(
          html`
            <settings-toggle-button
              class="hr"
              id="speedreader"
              pref="{{prefs.brave.speedreader.enabled}}"
              label="${loadTimeData.getString('speedreaderSettingLabel')}"
              sub-label=
                "${loadTimeData.getString('speedreaderSettingSubLabel')}"
              learn-more-url=
                "${loadTimeData.getString('speedreaderLearnMoreURL')}"
            </settings-toggle-button>
          `)
        fontsRow.parentNode.appendChild(
        html`
          <settings-toggle-button
            class="hr"
            id="mru-cycling"
            pref="{{prefs.brave.mru_cycling_enabled}}"
            label="${loadTimeData.getString('mruCyclingSettingLabel')}"
          </settings-toggle-button>
        `)
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
    // </if>
  },
})
