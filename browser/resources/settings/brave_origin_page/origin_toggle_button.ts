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
import * as BraveOriginMojom from '../brave_origin_settings.mojom-webui.js';

export class OriginToggleButtonElement extends PolymerElement {
  static get is() {
    return 'origin-toggle-button';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      policyKey: String,
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
      },
      disabled_: {
        type: Boolean,
        value: false,
      },
    };
  }

  declare policyKey: string;
  declare inverted: boolean;
  declare label: string;
  declare subLabel: string;
  declare icon: string;
  declare checked_: boolean;
  declare disabled_: boolean;
  private braveOriginHandler_: BraveOriginMojom.BraveOriginSettingsHandlerRemote;
  private isReverting_: boolean = false;

  get checked(): boolean {
    return this.checked_;
  }

  // Implement searchContents for settings search functionality
  searchContents(query: string): boolean {
    if (!query) {
      return true;
    }
    const lowerQuery = query.toLowerCase();
    return (this.label?.toLowerCase().includes(lowerQuery) || false) ||
           (this.subLabel?.toLowerCase().includes(lowerQuery) || false);
  }

  override ready() {
    super.ready();
    // Set defaults immediately so element renders
    this.checked_ = false;
    this.disabled_ = false;
    // Initialize the mojo handler
    this.braveOriginHandler_ = BraveOriginMojom.BraveOriginSettingsHandler.getRemote();
    // Then load actual values
    this.loadPolicyValue_();
  }

  async loadPolicyValue_() {
    if (!this.policyKey) {
      return;
    }

    try {
      const {isControlled} = await this.braveOriginHandler_.isPolicyControlledByBraveOrigin(this.policyKey);
      const {value} = await this.braveOriginHandler_.getPolicyValue(this.policyKey);

      // Disable if not controlled by Brave Origin
      this.disabled_ = !isControlled;

      if (value !== null) {
        // Apply inversion if needed
        this.checked_ = this.inverted ? !value : value;
      }
    } catch (e) {
      console.error('origin-toggle-button: Error loading policy value', e);
    }
  }

  private async onRowClick_() {
    if (this.disabled_) {
      return;
    }

    this.checked_ = !this.checked_;
    await this.handleToggleChange_();
  }

  private async handleToggleChange_() {
    // Prevent recursive calls when reverting
    if (this.isReverting_) {
      return;
    }

    if (!this.policyKey) {
      return;
    }

    // The checked_ value has already been updated by Polymer's two-way binding
    // Apply inversion when setting the value
    const valueToSet = this.inverted ? !this.checked_ : this.checked_;

    try {
      const {success} = await this.braveOriginHandler_.setPolicyValue(this.policyKey, valueToSet);

      if (!success) {
        // Revert on failure
        this.isReverting_ = true;
        this.checked_ = !this.checked_;
        this.isReverting_ = false;
      }
    } catch (error) {
      console.error('origin-toggle-button: Error setting policy value', error);
      // Revert on error
      this.isReverting_ = true;
      this.checked_ = !this.checked_;
      this.isReverting_ = false;
    }
  }
}

customElements.define(OriginToggleButtonElement.is, OriginToggleButtonElement);