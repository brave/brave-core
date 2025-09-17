// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import '/shared/settings/prefs/prefs.js';
import '../settings_shared.css.js';

import type { CrViewManagerElement } from 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { loadTimeData } from '../i18n_setup.js'

import { routes } from '../route.js';
import { RouteObserverMixin } from '../router.js';
import type { Route } from '../router.js';
import type { SettingsPlugin } from '../settings_main/settings_plugin.js';
import { SearchableViewContainerMixin } from '../settings_page/searchable_view_container_mixin.js';

import { getTemplate } from './brave_system_page_index.html.js';

// <if expr="enable_brave_vpn_wireguard">
import '../brave_system_page/brave_vpn_page.js'
// </if>
import '../shortcuts_page/shortcuts_page.js'

export interface SettingsBraveSystemPageIndexElement {
  $: {
    viewManager: CrViewManagerElement,
  };
}

const SettingsBraveExtensionsPageIndexElementBase =
  SearchableViewContainerMixin(RouteObserverMixin(PolymerElement));

export class SettingsBraveSystemPageIndexElement extends
  SettingsBraveExtensionsPageIndexElementBase implements SettingsPlugin {
  static get is() {
    return 'settings-brave-system-page-index';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      prefs: Object,
      showShortcutsPage_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('areShortcutsSupported'),
      },
    };
  }

  declare prefs: { [key: string]: any };
  declare private showShortcutsPage_: boolean;

  private showDefaultViews_() {
    const views = ['system']
    // <if expr="enable_brave_vpn_wireguard">
    if (loadTimeData.getBoolean('isBraveVPNEnabled')
      // <if expr="is_macosx">
      && loadTimeData.getBoolean('isBraveVPNWireguardEnabledOnMac')
      // </if>
    ) {
      views.push('vpn')
    }
    // </if>

    views.push('memory')
    this.$.viewManager.switchViews(views, 'no-animation', 'no-animation');
  }

  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    super.currentRouteChanged(newRoute, oldRoute);

    // Need to wait for currentRouteChanged observers on child views to run
    // first, before switching views.
    queueMicrotask(() => {
      switch (newRoute) {
        case routes.SYSTEM:
        case routes.BASIC:
          this.showDefaultViews_();
          break;
        case routes.SHORTCUTS:
          this.$.viewManager.switchView(
            'shortcuts', 'no-animation', 'no-animation');
          break;
      }
    });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-system-page-index': SettingsBraveSystemPageIndexElement;
  }
}

customElements.define(
  SettingsBraveSystemPageIndexElement.is, SettingsBraveSystemPageIndexElement);
