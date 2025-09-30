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

import {PerformanceBrowserProxy, PerformanceBrowserProxyImpl} from '../performance_page/performance_browser_proxy.js'

import { routes } from '../route.js';
import { RouteObserverMixin, Router } from '../router.js';
import type { Route } from '../router.js';
import type { SettingsPlugin } from '../settings_main/settings_plugin.js';
import { SearchableViewContainerMixin } from '../settings_page/searchable_view_container_mixin.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'

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
  SearchableViewContainerMixin(RouteObserverMixin(WebUiListenerMixin(PolymerElement)));

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
      /**
      * Used to hide battery settings section if the device has no battery
      */
      showBatterySettings_: {
        type: Boolean,
        value: false,
      },
      showShortcutsPage_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('areShortcutsSupported'),
      },
      // <if expr="enable_brave_vpn_wireguard">
      showVPNPage_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean('isBraveVPNEnabled')
          // <if expr="is_macosx">
          && loadTimeData.getBoolean('isBraveVPNWireguardEnabledOnMac')
          // </if>
      },
      // </if>
    };
  }

  declare prefs: { [key: string]: any };
  declare private showBatterySettings_: boolean;
  declare private showShortcutsPage_: boolean;

  // <if expr="enable_brave_vpn_wireguard">
  declare private showVPNPage_: boolean;
  // </if>

  private performanceBrowserProxy_: PerformanceBrowserProxy =
      PerformanceBrowserProxyImpl.getInstance();

  override connectedCallback() {
    super.connectedCallback();
    this.addWebUiListener(
      'device-has-battery-changed', this.hasBatteryChanged_)
    this.performanceBrowserProxy_.getDeviceHasBattery().then(this.hasBatteryChanged_)
  }

  private hasBatteryChanged_ = (hasBattery: boolean) => {
    this.showBatterySettings_ = hasBattery;

    // If we get informed whether we have a battery while on the system page,
    // show the default views again to show/hide the battery section.
    if (Router.getInstance().currentRoute === routes.SYSTEM) {
      this.showDefaultViews_();
    }
  }

  private showDefaultViews_() {
    const views = ['system']
    // <if expr="enable_brave_vpn_wireguard">
    if (this.showVPNPage_) {
      views.push('vpn')
    }
    // </if>

    views.push('memory')

    if (this.showBatterySettings_) {
      views.push('battery')
    }

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
