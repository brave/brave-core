// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/shared/settings/prefs/prefs.js';
import 'chrome://resources/cr_elements/cr_shared_style.css.js';
import '../settings_shared.css.js';
import '../settings_page/settings_subpage.js';
import './email_aliases_page.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './email_aliases_section.html.js';

export class SettingsEmailAliasesSectionElement extends PolymerElement {
  static get is() { return 'settings-email-aliases-section'; }
  static get template() { return getTemplate(); }
  static get properties() { return { prefs: Object } }
  declare prefs: {[key: string]: any};
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-email-aliases-section': SettingsEmailAliasesSectionElement;
  }
}

customElements.define(SettingsEmailAliasesSectionElement.is,
                      SettingsEmailAliasesSectionElement);
