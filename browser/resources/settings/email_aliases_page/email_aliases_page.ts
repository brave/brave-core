// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import '../settings_page/settings_subpage.js';
import '../settings_page/settings_section.js';
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';
import {EmailAliasesStrings} from '../brave_components_webui_strings.js';
import './email_aliases_autofill_toggle.js';
import {getTemplate} from './email_aliases_page.html.js';

// Unfortunately, our current WebPack build does not support ESModule output and
// it expects loadTimeData to be on the globalThis. The settings page has been
// migrated to use the ESModule version of loadTimeData and strings.m.js. For
// now, this provides a shim between the old and the new system.
window.loadTimeData = loadTimeData

interface SettingsEmailAliasesPageElement {
  $: {
    signInRoot: HTMLElement,
    manageSection: HTMLElement,
  }
}

class SettingsEmailAliasesPageElement extends PrefsMixin(PolymerElement) {
  static get is() {
    return 'settings-email-aliases-page';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      pageTitle_: {
        type: String,
        value: () => loadTimeData.getString(
            EmailAliasesStrings.SETTINGS_EMAIL_ALIASES_LABEL),
      },
      showAutofillToggle_: {
        type: Boolean,
        value: false,
      },
    };
  }

  declare pageTitle_: string;
  declare showAutofillToggle_: boolean;

  override ready() {
    super.ready();

    customElements.whenDefined('settings-brave-account-row').then(() => {
      const bundlePath = '/email_aliases.bundle.js'
      import(bundlePath).then(({mount}) => {
        mount(this.$.signInRoot, this.$.manageSection, {
          onLoggedInChange: (loggedIn: boolean) => {
            this.showAutofillToggle_ = loggedIn;
          },
        });
      });
    });

    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing[`emailAliases`] = this.shadowRoot
    }
  }
}

customElements.define(
    SettingsEmailAliasesPageElement.is, SettingsEmailAliasesPageElement);

declare global {
  interface HTMLElementTagNameMap {
    'settings-email-aliases-page': SettingsEmailAliasesPageElement;
  }
}
