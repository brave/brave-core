// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// The web-component-missing-deps lint rule requires the class-definition file
// (this override) to import every custom element referenced by the template
// (settings_section.html), so mirror the upstream element imports here.
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js';
import 'chrome://resources/cr_elements/cr_toast/cr_toast.js';
import './dialogs/disconnect_cloud_authenticator_dialog.js';
import './dialogs/remove_actor_login_permission_dialog.js';
import './full_data_reset.js';
import './passwords_exporter.js';
import './passwords_importer.js';
import './prefs/pref_toggle_button.js';
import './site_favicon.js';
import '/shared/settings/controls/extension_controlled_indicator.js';

import {html, RegisterPolymerTemplateModifications} from '//resources/brave/polymer_overriding.js'
import {loadTimeData} from '//resources/js/load_time_data.js'

RegisterPolymerTemplateModifications({
  'settings-section': (templateContent) => {
    const passwordToggle = templateContent.querySelector('#passwordToggle')
    if (!passwordToggle || !passwordToggle.parentNode) {
      console.error(
        `[Brave Password Manager Overrides] Could not find '#passwordToggle'`)
      return
    }
    // Toggle that gates Brave's built-in password autofill/save via the
    // brave.password_manager.fill_enabled pref. Turning it off suppresses both
    // the fill dropdown and the "offer to save password" prompt, so users of a
    // third-party password manager don't get two competing suggestions.
    passwordToggle.parentNode.insertBefore(
      html`
        <pref-toggle-button id="braveFillToggle" class="hr"
            label="${loadTimeData.getString('bravePasswordManagerFillLabel')}"
            sub-label="${
          loadTimeData.getString('bravePasswordManagerFillSubLabel')}"
            pref="{{prefs.brave.password_manager.fill_enabled}}">
        </pref-toggle-button>`,
      passwordToggle.nextSibling)
  }
})

export * from './settings_section-chromium.js'
