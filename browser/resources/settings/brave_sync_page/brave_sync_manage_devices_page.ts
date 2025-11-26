// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js';
import '../settings_vars.css.js';

import { I18nMixin, I18nMixinInterface } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import { BaseMixin } from '../base_mixin.js'
import { Route, Router } from '../router.js';
import { SettingsViewMixin, SettingsViewMixinInterface } from '../settings_page/settings_view_mixin.js';


import { getTemplate } from './brave_sync_manage_devices_page.html.js'

const SettingsBraveSyncManageDevicesPageElementBase =
  SettingsViewMixin(I18nMixin(BaseMixin(PolymerElement))) as new() =>
    PolymerElement & I18nMixinInterface & SettingsViewMixinInterface

export class SettingsBraveSyncManageDevicesPageElement extends SettingsBraveSyncManageDevicesPageElementBase {
  static get is() {
    return 'settings-brave-sync-manage-devices-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      label: {
        type: String,
      },
    };
  }

  declare label: string;

  onSyncTap_() {
    // Users can go to sync subpage regardless of sync status.
    const router = Router.getInstance();
    router.navigateTo(
      (router.getRoutes() as { BRAVE_SYNC_SETUP: Route }).BRAVE_SYNC_SETUP);
  }

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'setup':
        return this.shadowRoot!.querySelector('#brave-sync-setup')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
  }
}

customElements.define(
  SettingsBraveSyncManageDevicesPageElement.is, SettingsBraveSyncManageDevicesPageElement)

