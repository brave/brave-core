// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_toggle/cr_toggle.js';
import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/brave/leo.bundle.js';
import '../settings_shared.css.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './origin_toggle_button.html.js';

export class OriginToggleButtonElement extends PolymerElement {
  static get is() {
    return 'origin-toggle-button';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      policyKey: {
        type: String,
        reflectToAttribute: true,
      },
      inverted: {
        type: Boolean,
        value: false,
      },
      label: String,
      subLabel: String,
      icon: String,
      checked_: {
        type: Boolean,
        value: false,
        observer: 'onCheckedChanged_',
      },
    };
  }

  declare policyKey: string;
  declare inverted: boolean;
  declare label: string;
  declare subLabel: string;
  declare icon: string;
  declare checked_: boolean;

  get checked(): boolean {
    return this.checked_;
  }

  override ready() {
    super.ready();
  }

  private onRowClick_() {
  }

  private async onCheckedChanged_(_newValue: boolean, oldValue: boolean) {
  }

  private async handleToggleChange_() {
  }
}

customElements.define(OriginToggleButtonElement.is, OriginToggleButtonElement);
