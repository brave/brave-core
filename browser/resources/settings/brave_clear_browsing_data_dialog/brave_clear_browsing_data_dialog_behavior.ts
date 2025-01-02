// @ts-nocheck
/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */


import "./brave_clear_browsing_data_on_exit_page.js"

import {loadTimeData} from "../i18n_setup.js"
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {SettingsClearBrowsingDataDialogElement} from '../clear_browsing_data_dialog/clear_browsing_data_dialog.js'
import type {SettingsClearBrowsingDataDialogElement as BraveSettingsClearBrowsingDataDialogElement}
    from '../clear_browsing_data_dialog/clear_browsing_data_dialog.js'
import {BraveClearBrowsingDataDialogBrowserProxy, BraveClearBrowsingDataDialogBrowserProxyImpl}
    from './brave_clear_browsing_data_dialog_proxy.js'

const BaseElement = WebUiListenerMixin(SettingsClearBrowsingDataDialogElement)
export class BraveSettingsClearBrowsingDataDialogElement extends BaseElement {
  braveRewardsEnabled_: Boolean = false;
  onClearBraveAdsDataClickHandler_: (() => void) = () => {};

  private clearDataBrowserProxy_ =
      BraveClearBrowsingDataDialogBrowserProxyImpl.getInstance();
  private onSelectedPageChangedCallback_: (() => void) | null = null;
  private updateSaveButtonStateCallback_: (() => void) | null = null;
  private saveOnExitSettingsCallback_: (() => void) | null = null;

  override ready() {
    super.ready();

    // Append On exit tab to tab selector.
    this.tabsNames_.push(loadTimeData.getString('onExitPageTitle'));

    this.addWebUiListener(
      'update-counter-text', this.updateOnExitCountersText.bind(this))

    this.addWebUiListener(
      'brave-rewards-enabled-changed', (enabled: boolean) => {
      this.braveRewardsEnabled_ = enabled
    })

    this.clearDataBrowserProxy_.getBraveRewardsEnabled().then((enabled) => {
      this.braveRewardsEnabled_ = enabled
    })
  }

  override connectedCallback() {
    super.connectedCallback()

    this.onSelectedPageChangedCallback_ = this.onSelectedPageChanged_.bind(this)
    this.$.pages.addEventListener('iron-select',
      this.onSelectedPageChangedCallback_)

    this.updateSaveButtonStateCallback_ = this.updateSaveButtonState_.bind(this)
    this.shadowRoot!.querySelector('#on-exit-tab')!.addEventListener(
      'clear-data-on-exit-page-change', this.updateSaveButtonStateCallback_)

    this.saveOnExitSettingsCallback_ = this.saveOnExitSettings_.bind(this);
    this.shadowRoot.querySelector('#saveOnExitSettingsConfirm').addEventListener(
      'click', this.saveOnExitSettingsCallback_);

    this.onClearBraveAdsDataClickHandler_ = this.clearBraveAdsData_.bind(this)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    if (this.onSelectedPageChangedCallback_) {
      this.$.pages.removeEventListener('selected-item-changed',
        this.onSelectedPageChangedCallback_)
      this.onSelectedPageChangedCallback_ = null
    }

    if (this.saveOnExitSettingsCallback_) {
      this.shadowRoot.querySelector('#on-exit-tab').removeEventListener(
        'clear-data-on-exit-page-change', this.updateSaveButtonStateCallback_);
      this.updateSaveButtonStateCallback_ = null;
    }

    if (this.saveOnExitSettingsCallback_) {
      this.shadowRoot.querySelector('#saveOnExitSettingsConfirm').removeEventListener(
        'click', this.saveOnExitSettingsCallback_);
      this.saveOnExitSettingsCallback_ = null;
    }

    this.onClearBraveAdsDataClickHandler_ = () => {}
  }

/**
  * Updates the text of a browsing data counter corresponding to the given
  * preference.
  * @param {string} prefName Browsing data type deletion preference.
  * @param {string} text The text with which to update the counter
  * @private
  */
  updateOnExitCountersText(prefName, text) {
    // Data type deletion preferences are named "browser.clear_data.<datatype>".
    // Strip the common prefix, i.e. use only "<datatype>".
    const matches = prefName.match(/^browser\.clear_data\.(\w+)$/);
    this.shadowRoot.querySelector('#on-exit-tab').setCounter(matches[1], text);
  }

  /**
   * Updates Clear and Save buttons visibility based on the selected tab.
   * @private
   */
  onSelectedPageChanged_() {
    const page = this.$.pages.selectedItem;
    if (!page) {
      return
    }
    const isOnExitPage = (page.id === 'on-exit-tab');
    this.$.clearButton.hidden = isOnExitPage;
    this.shadowRoot.querySelector('#saveOnExitSettingsConfirm').hidden = !isOnExitPage;
  }

  /**
   * Updates Save button enabled state based on on-exit-tab's changed state.
   * @private
   */
  updateSaveButtonState_() {
    this.shadowRoot.querySelector('#saveOnExitSettingsConfirm').disabled =
        !this.shadowRoot.querySelector('#on-exit-tab').isModified;
  }

  /**
   * Saves on exit settings selections.
   * @private
   */
  saveOnExitSettings_() {
    const changed = this.shadowRoot.querySelector('#on-exit-tab').getChangedSettings();
    changed.forEach((change) => {
      this.set('prefs.' + change.key + '.value', change.value)
    })
    this.updateSaveButtonState_()
    if (!this.clearingInProgress_) {
      this.$.clearBrowsingDataDialog.close()
    }
  }

  /**
   * Clears Brave Ads data.
   * @private
   */
  clearBraveAdsData_(e: any) {
    e.preventDefault();
    this.clearDataBrowserProxy_.clearBraveAdsData();
    this.$.clearBrowsingDataDialog.close();
  }
}
