// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/shared/settings/prefs/prefs.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/brave/leo.bundle.js';

import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {getTemplate} from './brave_origin_page.html.js'
import * as BraveOriginMojom from '../brave_origin_settings.mojom-webui.js'
import './origin_toggle_button.js'
import type {OriginToggleButtonElement} from './origin_toggle_button.js'

const SettingsBraveOriginPageElementBase =
  PrefsMixin(BaseMixin(I18nMixin(WebUiListenerMixin(
    PolymerElement)))) as {
    new(): PolymerElement &
           PrefsMixinInterface &
           WebUiListenerMixinInterface &
           I18nMixinInterface
  }

/**
 * 'settings-brave-origin-page' is the settings page containing
 * Brave Origin features.
 */
export class SettingsBraveOriginPageElement
    extends SettingsBraveOriginPageElementBase {

  static get is() {
    return 'settings-brave-origin-page'
  }

  static get template() {
    return getTemplate()
  }

  private braveOriginHandler_: BraveOriginMojom.BraveOriginSettingsHandlerRemote

  override ready() {
    super.ready()

    // Initialize the mojo handler
    this.braveOriginHandler_ =
        BraveOriginMojom.BraveOriginSettingsHandler.getRemote()
  }

  private async resetToDefaults_() {
    // Query all origin-toggle-button elements
    const toggles = this.shadowRoot!.querySelectorAll('origin-toggle-button');

    // Turn off all toggles that are currently on
    for (const toggle of toggles) {
      const toggleElement = toggle as OriginToggleButtonElement;
      if (toggleElement.checked) {
        // Set to off (accounting for inverted toggles)
        const valueToSet = toggleElement.inverted ? true : false;
        await this.braveOriginHandler_.setPolicyValue(
            toggleElement.policyKey, valueToSet);
      }
    }

    // Reload all toggle states
    for (const toggle of toggles) {
      await (toggle as OriginToggleButtonElement).loadPolicyValue_();
    }
  }
}

customElements.define(
    SettingsBraveOriginPageElement.is, SettingsBraveOriginPageElement);
