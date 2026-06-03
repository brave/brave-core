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
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RelaunchMixin, RelaunchMixinInterface, RestartType} from '../relaunch_mixin.js'
import '../relaunch_confirmation_dialog.js'

import {getTemplate} from './brave_origin_page.html.js'
import {getSearchManager} from '../search_settings.js'
import type {SearchResult} from '../search_settings.js'
import type {SettingsPlugin} from '../settings_main/settings_plugin.js'
import * as BraveOriginMojom from '../brave_origin_settings.mojom-webui.js'
import './brave_origin_onboarding.js'
import './origin_toggle_button.js'
import type {OriginToggleButtonElement} from './origin_toggle_button.js'

const SettingsBraveOriginPageElementBase =
  RelaunchMixin(PrefsMixin(BaseMixin(I18nMixin(WebUiListenerMixin(
    PolymerElement))))) as {
    new(): PolymerElement &
           PrefsMixinInterface &
           WebUiListenerMixinInterface &
           I18nMixinInterface &
           RelaunchMixinInterface
  }

/**
 * 'settings-brave-origin-page' is the settings page containing
 * Brave Origin features.
 */
export class SettingsBraveOriginPageElement
    extends SettingsBraveOriginPageElementBase implements SettingsPlugin {

  static get is() {
    return 'settings-brave-origin-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      isPurchased_: {
        type: Boolean,
        value: true,
      },
      showRestartToast_: {
        type: Boolean,
        value: false,
      },
      isPlaylistFeatureEnabled_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('isPlaylistFeatureEnabled'),
      },
    }
  }

  declare private isPurchased_: boolean
  declare private showRestartToast_: boolean
  declare private isPlaylistFeatureEnabled_: boolean
  private braveOriginHandler_: BraveOriginMojom.BraveOriginSettingsHandlerRemote
  private boundOnVisibilityChange_: (() => void) | null = null
  private boundOnPolicyValueChanged_: (() => void) | null = null

  override ready() {
    super.ready()

    // Initialize the mojo handler
    this.braveOriginHandler_ =
        BraveOriginMojom.BraveOriginSettingsHandler.getRemote()

    // Check if a restart is needed from a prior settings change
    this.checkNeedsRestart_()

    // Re-check restart state when a toggle changes
    this.boundOnPolicyValueChanged_ = this.checkNeedsRestart_.bind(this)
    this.addEventListener('policy-value-changed',
        this.boundOnPolicyValueChanged_)

    // For branded builds, always show as purchased
    if (loadTimeData.getBoolean('isBraveOriginBrandedBuild')) {
      this.isPurchased_ = true
      return
    }

    // Check purchase state
    this.checkPurchaseState_()

    // Handle proceed-free from onboarding component
    this.addEventListener('proceed-free-clicked',
        () => this.onProceedFreeClicked_())

    // Re-check when the tab becomes visible (user may return from
    // account page after purchasing)
    this.boundOnVisibilityChange_ = this.onVisibilityChange_.bind(this)
    document.addEventListener('visibilitychange',
        this.boundOnVisibilityChange_)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (this.boundOnPolicyValueChanged_) {
      this.removeEventListener('policy-value-changed',
          this.boundOnPolicyValueChanged_)
      this.boundOnPolicyValueChanged_ = null
    }
    if (this.boundOnVisibilityChange_) {
      document.removeEventListener('visibilitychange',
          this.boundOnVisibilityChange_)
      this.boundOnVisibilityChange_ = null
    }
  }

  private onVisibilityChange_() {
    if (document.visibilityState === 'visible') {
      this.checkPurchaseState_()
    }
  }

  private async checkPurchaseState_() {
    const {isPurchased} =
        await this.braveOriginHandler_.refreshPurchaseState()
    this.isPurchased_ = isPurchased
  }

  private async onProceedFreeClicked_() {
    await this.braveOriginHandler_.proceedFree()
    this.isPurchased_ = true
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

    // Check if restart is needed after reset
    this.checkNeedsRestart_()
  }

  private async checkNeedsRestart_() {
    const {needsRestart} =
        await this.braveOriginHandler_.getNeedsRestart()
    this.showRestartToast_ = needsRestart
  }

  private getOriginRestartPaddingClass_(showRestartToast: boolean): string {
    return showRestartToast ? 'origin-restart-padding-spacer' : ''
  }

  private restartBrowser_(e: Event) {
    e.stopPropagation()
    this.performRestart(RestartType.RESTART)
  }

  async searchContents(query: string): Promise<SearchResult> {
    const searchRequest = await getSearchManager().search(query, this)
    return searchRequest.getSearchResult()
  }
}

customElements.define(
    SettingsBraveOriginPageElement.is, SettingsBraveOriginPageElement);
