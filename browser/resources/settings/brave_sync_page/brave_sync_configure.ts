// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import './brave_sync_code_dialog.js';

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUIListenerMixin, WebUIListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {Route,Router} from '../router.js';
import {BraveDeviceInfo, BraveSyncBrowserProxy} from './brave_sync_browser_proxy.js';
import {SyncStatus} from '../people_page/sync_browser_proxy.js';
import {getTemplate} from './brave_sync_configure.html.js'

/**
 * @fileoverview
 * 'settings-brave-sync-configure' is the set of controls which fetches, displays
 * and updates the sync configuration.
 */

const SettingsBraveSyncConfigureElementBase =
  I18nMixin(WebUIListenerMixin(BaseMixin(PolymerElement))) as {
    new(): PolymerElement & WebUIListenerMixinInterface & I18nMixinInterface
  }

export class SettingsBraveSyncConfigureElement extends SettingsBraveSyncConfigureElementBase {
  static get is() {
    return 'settings-brave-sync-configure'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /**
       * The current sync status, supplied by parent element.
       * @type {!SyncStatus}
       */
      syncStatus: Object,
      /**
       * Configured sync code
       */
      syncCode: {
        type: String,
        notify: true
      },
      /**
       * List of devices in sync chain
       * @private
       */
      deviceList_: Array,
      /**
       * Sync code dialog type. Can only have 1 at a time, so use a single property.
       * 'qr' | 'words' | 'input' | 'choose' | null
       * @private
       */
      syncCodeDialogType_: String,
    };
  }

  private syncStatus: SyncStatus;
  private syncCode: string | undefined;
  private deviceList_: BraveDeviceInfo[];
  private syncCodeDialogType_: string | null;

  browserProxy_: BraveSyncBrowserProxy = BraveSyncBrowserProxy.getInstance();

  override async connectedCallback() {
    super.connectedCallback()
    const deviceList = await this.browserProxy_.getDeviceList()
    this.addWebUIListener('device-info-changed', this.handleDeviceInfo_.bind(this))
    this.handleDeviceInfo_(deviceList)
  }

  handleDeviceInfo_(deviceList: BraveDeviceInfo[]) {
    this.deviceList_ = deviceList
  }

  getDeviceDisplayDate(device: BraveDeviceInfo): string {
    const displayDate = new Date(0)
    displayDate.setUTCSeconds(device.lastUpdatedTimestamp)
    return displayDate.toDateString()
  }

  /*
   * This only sets sync code when it is not already set. It needs to be cleared
   * when sync chain reset
   */
  async ensureSetSyncCode_() {
    if (!!this.syncCode)
      return
    const syncCode = await this.browserProxy_.getSyncCode()
    this.syncCode = syncCode
  }

  async onViewSyncCode_() {
    await this.ensureSetSyncCode_()
    this.syncCodeDialogType_ = 'words'
  }

  async onAddDevice_() {
    await this.ensureSetSyncCode_()
    this.syncCodeDialogType_ = 'choose'
  }

  onSyncCodeDialogDone_() {
    this.syncCodeDialogType_ = null
  }

  async onResetSyncChain_() {
    const messageText = this.i18n('braveSyncResetConfirmation')
    const shouldReset = confirm(messageText)
    if (!shouldReset) {
      return
    }
    await this.browserProxy_.resetSyncChain();
    // Clear sync code because user might use the same page to create a new sync
    // chain without reload
    this.syncCode = undefined
    const router = Router.getInstance();
    router.navigateTo((router.getRoutes() as {BRAVE_SYNC: Route}).BRAVE_SYNC);
  }

  async onDeleteDevice_(e: Event) {
    const messageText = this.i18n('braveSyncDeleteDeviceConfirmation')
    const shouldDeleteDevice = confirm(messageText)
    if (!shouldDeleteDevice) {
      return
    }

    const deviceGuid = (e.currentTarget as HTMLElement).getAttribute('arg')!;
    await this.browserProxy_.deleteDevice(deviceGuid);
  }
}

customElements.define(
  SettingsBraveSyncConfigureElement.is, SettingsBraveSyncConfigureElement)
