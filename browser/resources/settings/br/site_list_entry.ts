// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { SiteListEntryElement } from '../site_settings/site_list_entry.js'

import {
  RegisterPolymerComponentReplacement,
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'

RegisterStyleOverride(
  'site-list-entry',
  html`
    <style>
      #resetSite[disabled] {
        pointer-events: all;
        cursor: not-allowed;
      }
    </style>
  `
)

RegisterPolymerTemplateModifications({
  'site-list-entry': (templateContent) => {
    const resetSiteTemplate = templateContent.querySelector(
        'template[is="dom-if"][if="[[shouldShowResetButton_(model, readOnlyList)]]"]')
    if (!resetSiteTemplate) {
      console.error('[Settings] Could not find resetSite template')
      return
    } else {
      const resetSite = resetSiteTemplate.content.querySelector('#resetSite')
      if (!resetSite) {
        console.error('[Settings] Could not find reset site button')
        return
      }
      resetSite.setAttribute('disabled', '[[shouldDisableReset_(model)]]')
      resetSite.setAttribute('on-mouseenter', 'onResetSiteShowTooltip_')
      resetSite.setAttribute('iron-icon', 'trash')
      resetSite.removeAttribute('class')
    }
  }
})

enum BraveCookieTypes {
  SHIELDS_DOWN = 'shields down',
  SHIELDS_SETTINGS = 'shields settings',
  GOOGLE_SIGN_IN = 'google sign-in'
}

RegisterPolymerComponentReplacement(
  'site-list-entry',
  class BraveSiteListEntryElement extends SiteListEntryElement {
    private shouldDisableReset_() {
      return this.model.braveCookieType !== undefined
    }

    private onResetSiteShowTooltip_() {
      const button = this.shadowRoot!.querySelector('#resetSite')

      if (
        this.model.braveCookieType === BraveCookieTypes.SHIELDS_SETTINGS ||
        this.model.braveCookieType === BraveCookieTypes.SHIELDS_DOWN
      ) {
        const text = this.i18n('cookieControlledByShieldsTooltip')
        this.fire('show-tooltip', { target: button, text })
      } else if (
        this.model.braveCookieType === BraveCookieTypes.GOOGLE_SIGN_IN
      ) {
        const text = this.i18n('cookieControlledByGoogleSigninTooltip')
        this.fire('show-tooltip', { target: button, text })
      }
    }
  }
)
