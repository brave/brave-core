// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/cr_elements/cr_link_row/cr_link_row.js';
import 'chrome://resources/brave/leo.bundle.js';
import '../relaunch_confirmation_dialog.js';
import './brave_origin_onboarding.js';
import './origin_toggle_button.js';

import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {CrLitElement} from 'chrome://resources/lit/v3_0/lit.rollup.js';

import {RelaunchMixinLit, RestartType} from '../relaunch_mixin_lit.js';
import {getSearchManager} from '../search_settings.js';
import type {SearchResult} from '../search_settings.js';
import type {SettingsPlugin} from '../settings_main/settings_plugin.js';
import * as BraveOriginMojom from '../brave_origin_settings.mojom-webui.js';
import {getCss} from './brave_origin_page.css.js';
import {getHtml} from './brave_origin_page.html.js';
import type {OriginToggleButtonElement} from './origin_toggle_button.js';

const SettingsBraveOriginPageElementBase = RelaunchMixinLit(CrLitElement);

/**
 * 'settings-brave-origin-page' is the settings page containing
 * Brave Origin features.
 */
export class SettingsBraveOriginPageElement
    extends SettingsBraveOriginPageElementBase implements SettingsPlugin {
  static get is() {
    return 'settings-brave-origin-page';
  }

  static override get styles() {
    return getCss();
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      isPurchased_: {type: Boolean},
      showRestartToast_: {type: Boolean},
      isPlaylistFeatureEnabled_: {type: Boolean},
      // <if expr="enable_psst">
      isPsstEnabled_: {type: Boolean},
      // </if>
    };
  }

  accessor isPurchased_: boolean = true;
  accessor showRestartToast_: boolean = false;
  accessor isPlaylistFeatureEnabled_: boolean =
      loadTimeData.getBoolean('isPlaylistFeatureEnabled');
  // <if expr="enable_psst">
  accessor isPsstEnabled_: boolean = loadTimeData.getBoolean('isPsstEnabled');
  // </if>
  private braveOriginHandler_ =
      BraveOriginMojom.BraveOriginSettingsHandler.getRemote();
  private boundOnVisibilityChange_: (() => void) | null = null;

  override connectedCallback() {
    super.connectedCallback();

    // Check if a restart is needed from a prior settings change
    this.checkNeedsRestart_();

    // For branded builds, always show as purchased
    if (loadTimeData.getBoolean('isBraveOriginBrandedBuild')) {
      this.isPurchased_ = true;
      return;
    }

    // Check purchase state
    this.checkPurchaseState_();

    // Re-check when the tab becomes visible (user may return from
    // account page after purchasing)
    this.boundOnVisibilityChange_ = this.onVisibilityChange_.bind(this);
    document.addEventListener('visibilitychange',
        this.boundOnVisibilityChange_);
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    if (this.boundOnVisibilityChange_) {
      document.removeEventListener('visibilitychange',
          this.boundOnVisibilityChange_);
      this.boundOnVisibilityChange_ = null;
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

  async onResetToDefaultsClick_() {
    // Query all origin-toggle-button elements
    const toggles = this.shadowRoot.querySelectorAll('origin-toggle-button');

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

  onPolicyValueChanged_() {
    this.checkNeedsRestart_();
  }

  private async checkNeedsRestart_() {
    const {needsRestart} =
        await this.braveOriginHandler_.getNeedsRestart()
    this.showRestartToast_ = needsRestart
  }

  getOriginRestartPaddingClass_(showRestartToast: boolean): string {
    return showRestartToast ? 'origin-restart-padding-spacer' : ''
  }

  onRestartClick_(e: Event) {
    e.stopPropagation()
    this.performRestart(RestartType.RESTART)
  }

  async searchContents(query: string): Promise<SearchResult> {
    const searchRequest = await getSearchManager().search(query, this)
    return searchRequest.getSearchResult()
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-origin-page': SettingsBraveOriginPageElement;
  }
}

customElements.define(
    SettingsBraveOriginPageElement.is, SettingsBraveOriginPageElement);
