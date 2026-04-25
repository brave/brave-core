// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerTemplateModifications,
  RegisterPolymerPrototypeModification
} from 'chrome://resources/brave/polymer_overriding.js'
import { loadTimeData } from '../i18n_setup.js'

import { Router } from '../router.js'

RegisterPolymerPrototypeModification({
  'settings-system-page': (prototype) => {
    prototype.getAssociatedControlFor = function(childViewId: string) {
      if (childViewId === 'shortcuts') {
        return this.shadowRoot.querySelector('#shortcutsButton')!
      }
    }

    prototype.onShortcutsClicked_ = () => {
      const router = Router.getInstance()
      router.navigateTo((router.getRoutes() as any).SHORTCUTS)
    }
  }
})

RegisterPolymerTemplateModifications({
  'settings-system-page': (templateContent) => {
    const settingsSection = templateContent.querySelector('settings-section')
    if (!settingsSection) {
      throw new Error('[Settings] Missing settings-section on system page')
    }
    settingsSection.classList.remove('cr-centered-card-container')

    if (loadTimeData.getBoolean('areShortcutsSupported')) {
      settingsSection.prepend(html`
        <cr-link-row
          on-click="onShortcutsClicked_"
          id="shortcutsButton"
          label=${loadTimeData.getString('braveShortcutsPage')}
          role-description=
            ${loadTimeData.getString('subpageArrowRoleDescription')}>
          <span id="shortcutsButtonSubLabel" slot="sub-label">
        </cr-link-row>
        <div class="hr"></div>`)
    }

    settingsSection.appendChild(
      html`
        <settings-toggle-button
          class="cr-row"
          pref="{{prefs.brave.enable_closing_last_tab}}"
          label="${loadTimeData.getString('braveClosingLastTab')}"
        >
        </settings-toggle-button>
      `
    )

    settingsSection.appendChild(
      html`
        <settings-toggle-button
          class="cr-row hr"
          pref="{{prefs.brave.enable_window_closing_confirm}}"
          label="${loadTimeData.getString('braveWarnBeforeClosingWindow')}">
        </settings-toggle-button>
      `
    )

    // <if expr="is_macosx">
    settingsSection.appendChild(
      html`
        <settings-toggle-button
          class="cr-row hr"
          pref="{{prefs.browser.confirm_to_quit}}"
          label="${loadTimeData.getString('warnBeforeQuitting')}">
        </settings-toggle-button>
      `
    )
    // </if>

    settingsSection.appendChild(
      html`
        <settings-toggle-button
          class="cr-row"
          pref="{{prefs.brave.show_fullscreen_reminder}}"
          label="${loadTimeData.getString('braveShowFullscreenReminder')}"
        >
        </settings-toggle-button>
      `
    )
  }
})
