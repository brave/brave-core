// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'
import {Router} from '../router.m.js'
import {loadTimeData} from '../i18n_setup.js'

import '../brave_appearance_page/super_referral.m.js'

RegisterPolymerTemplateModifications({
  'settings-appearance-page': (templateContent) => {
    // W/o super referral, we don't need to themes link option with themes sub
    // page.
    if (!loadTimeloadTimeData.getString('superReferralThemeName') === '')
      return

    // Routes
    const r = Router.getInstance().routes_
    if (!r.APPEARANCE) {
      console.error('[Brave Settings Overrides] Routes: could not find APPEARANCE page')
    }
    r.THEMES = r.APPEARANCE.createChild('/themes');
    // Hide chromium's theme section. It's replaced with our themes page.
    const theme = templateContent.getElementById('themeRow')
    theme.setAttribute('hidden', 'true')
    const pages = templateContent.getElementById('pages')
    const themes = document.createElement('template')
    themes.setAttribute('is', 'dom-if')
    themes.setAttribute('route-path', '/themes')
    themes.innerHTML = `
      <settings-subpage
          associated-control="[[$$('#themes-subpage-trigger')]]"
          page-title="${I18nBehavior.i18n('themes')}">
        <settings-brave-appearance-super-referral prefs="{{prefs}}">
        </settings-brave-appearance-super-referral>
      </settings-subpage> `
    pages.appendChild(themes)
  },
})
