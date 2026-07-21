// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import '../controls/settings_toggle_button.js';
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';
import {EmailAliasesStrings} from '../brave_components_webui_strings.js';
import {getTemplate} from './email_aliases_autofill_toggle.html.js';

class SettingsEmailAliasesAutofillToggleElement extends
    PrefsMixin(PolymerElement) {
  static get is() {
    return 'settings-email-aliases-autofill-toggle';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      label_: {
        type: String,
        value: () => loadTimeData.getString(
            EmailAliasesStrings
                .SETTINGS_EMAIL_ALIASES_AUTOFILL_SUGGESTION_LABEL),
      },
      subLabel_: {
        type: String,
        value: () => loadTimeData.getString(
            EmailAliasesStrings
                .SETTINGS_EMAIL_ALIASES_AUTOFILL_SUGGESTION_SUBLABEL),
      },
    };
  }

  declare label_: string;
  declare subLabel_: string;
}

customElements.define(
    SettingsEmailAliasesAutofillToggleElement.is,
    SettingsEmailAliasesAutofillToggleElement);

declare global {
  interface HTMLElementTagNameMap {
    'settings-email-aliases-autofill-toggle':
        SettingsEmailAliasesAutofillToggleElement;
  }
}
