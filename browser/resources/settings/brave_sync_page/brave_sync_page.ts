/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/icons.html.js';
import '../settings_page/settings_animated_pages.js';
import '../settings_page/settings_subpage.js';
import '../settings_shared.css.js';
import '../settings_vars.css.js';
import './brave_sync_subpage.js';

import {SyncBrowserProxy, SyncBrowserProxyImpl, SyncPrefs} from '/shared/settings/people_page/sync_browser_proxy.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {BaseMixin} from '../base_mixin.js'
import {Route, Router} from '../router.js';

import {BraveSyncBrowserProxy, BraveSyncStatus} from './brave_sync_browser_proxy.js';
import {getTemplate} from './brave_sync_page.html.js'

/**
 * @fileoverview
 * 'settings-brave-sync-page' is the settings page containing brave's
 * custom sync.
 */

const SettingsBraveSyncPageElementBase =
  I18nMixin(WebUiListenerMixin(BaseMixin(PolymerElement))) as {
    new(): PolymerElement & WebUiListenerMixinInterface & I18nMixinInterface
  }

export class SettingsBraveSyncPageElement extends SettingsBraveSyncPageElementBase {
  static get is() {
    return 'settings-brave-sync-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /**
       * The current sync status, supplied by SyncBrowserProxy.
       * @type {?SyncStatus}
       */
      syncStatus_: Object,
      isEncryptionSet_: {
        type: Boolean,
        value: false
      },
      syncLabel_: {
        type: String,
        computed: 'computeSyncLabel_(syncStatus_)'
      },
    };
  }

  private syncStatus_: BraveSyncStatus;
  private isEncryptionSet_: boolean;
  private syncLabel_: string;

  browserProxy_: SyncBrowserProxy = SyncBrowserProxyImpl.getInstance();
  braveBrowserProxy_: BraveSyncBrowserProxy = BraveSyncBrowserProxy.getInstance();

  computeSyncLabel_() {
    if (this.syncStatus_ !== undefined &&
        this.syncStatus_.hasSyncWordsDecryptionError) {
        return this.i18n('braveSyncCouldNotSyncActionLabel');
    }
    const isAlreadySetup = this.syncStatus_ !== undefined &&
        !this.syncStatus_.firstSetupInProgress;
    const key = isAlreadySetup ? 'braveSyncManageActionLabel' : 'braveSyncSetupActionLabel';
    return this.i18n(key);
  }

  override connectedCallback() {
    super.connectedCallback()
    const onSyncStatus = this.handleSyncStatus_.bind(this)
    this.braveBrowserProxy_.getSyncStatus().then(
        (status: BraveSyncStatus) => onSyncStatus(status));
    this.addWebUiListener(
      'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));
    this.addWebUiListener('sync-status-changed', onSyncStatus);
  }

  onSyncTap_() {
    // Users can go to sync subpage regardless of sync status.
    const router = Router.getInstance();
    router.navigateTo(
      (router.getRoutes() as {BRAVE_SYNC_SETUP: Route}).BRAVE_SYNC_SETUP);
  }

  /**
   * Handler for when the sync state is pushed from the browser.
   */
  async handleSyncStatus_(syncStatus: BraveSyncStatus) {
    this.syncStatus_ = syncStatus;
  }

  /**
   * Handler for when the sync preferences are updated.
   */
  async handleSyncPrefsChanged_(syncPrefs: SyncPrefs) {
    if (this.syncStatus_ && !this.syncStatus_.firstSetupInProgress) {
      const pureSyncCode = await this.braveBrowserProxy_.getPureSyncCode()
      if (syncPrefs.passphraseRequired) {
        await this.browserProxy_.setDecryptionPassphrase(pureSyncCode);
      } else if (!this.isEncryptionSet_) {
        this.browserProxy_.setEncryptionPassphrase(pureSyncCode)
        .then(successfullySet => {
          this.isEncryptionSet_ = successfullySet
        })
      }
    }
  }
}

customElements.define(
  SettingsBraveSyncPageElement.is, SettingsBraveSyncPageElement)
