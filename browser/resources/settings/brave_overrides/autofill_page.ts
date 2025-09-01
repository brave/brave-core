// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerTemplateModifications,
  RegisterPolymerComponentReplacement,
  RegisterPolymerPrototypeModification
} from 'chrome://resources/brave/polymer_overriding.js'
import {
  BraveSettingsAutofillPageElement
} from '../brave_autofill_page/brave_autofill_page.js'
import { loadTimeData } from '../i18n_setup.js'
import { routes } from '../route.js'
import type { Route } from '../router.js'
import '../email_aliases_page/email_aliases_page.js'

RegisterPolymerComponentReplacement(
  'settings-autofill-page', BraveSettingsAutofillPageElement
)

RegisterPolymerPrototypeModification({
  'settings-autofill-page-index': (prototype) => {
    const originalCurrentRouteChanged = prototype.currentRouteChanged
    prototype.currentRouteChanged =
      function (newRoute: Route, oldRoute?: Route) {
      if (newRoute === routes.EMAIL_ALIASES) {
        this.$.viewManager.switchView('email-aliases', 'no-animation',
                                      'no-animation')
        return
      }
      originalCurrentRouteChanged.call(this, newRoute, oldRoute)
    }
  }
})

RegisterPolymerTemplateModifications({
  'settings-autofill-page': (templateContent) => {
    const controlsDiv = templateContent.querySelector('div[route-path=default]')
    if (!controlsDiv) {
      throw new Error(
        '[Settings] Unable to find div[route-path=default] on autofill-page')
    }
    controlsDiv.appendChild(html`
        <settings-toggle-button
          class="hr"
          label="${loadTimeData.getString('autofillInPrivateSettingLabel')}"
          sub-label="${loadTimeData.getString('autofillInPrivateSettingDesc')}"
          id="autofill-private-profies"
          pref="{{prefs.brave.autofill_private_windows}}"
          hidden=[[!isAutofillPage_]]
        </settings-toggle-button>
      `)
  },
  'settings-autofill-page-index': (templateContent) => {
    if (!loadTimeData.getBoolean('isEmailAliasesEnabled')) {
      return
    }
    const viewManager = templateContent.querySelector('cr-view-manager')
    if (!viewManager) {
      throw new Error(
        '[Settings] Unable to find cr-view-manager on autofill-page')
    }
    viewManager.append(html`
      <settings-email-aliases-page page-title="Email Aliases" slot="view"
         id="email-aliases" data-parent-view-id="parent">
      </settings-email-aliases-page>
    `)
  }
})
