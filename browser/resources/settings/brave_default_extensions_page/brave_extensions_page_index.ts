// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import '/shared/settings/prefs/prefs.js';
import './brave_default_extensions_page.js';
import './brave_extensions_manifest_v2_subpage.js';
import '../settings_shared.css.js';

import type { CrViewManagerElement } from 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import { routes } from '../route.js';
import { RouteObserverMixin } from '../router.js';
import type { Route } from '../router.js';
import type { SettingsPlugin } from '../settings_main/settings_plugin.js';
import { SearchableViewContainerMixin } from '../settings_page/searchable_view_container_mixin.js';

import { getTemplate } from './brave_extensions_page_index.html.js';


export interface SettingsBraveExtensionsPageIndexElement {
  $: {
    viewManager: CrViewManagerElement,
  };
}

const SettingsBraveExtensionsPageIndexElementBase =
  SearchableViewContainerMixin(RouteObserverMixin(PolymerElement));

export class SettingsBraveExtensionsPageIndexElement extends
  SettingsBraveExtensionsPageIndexElementBase implements SettingsPlugin {
  static get is() {
    return 'settings-brave-extensions-page-index';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      prefs: Object,
    };
  }

  declare prefs: { [key: string]: any };

  private showDefaultViews_() {
    this.$.viewManager.switchViews(
      ['extensions'], 'no-animation', 'no-animation');
  }

  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    super.currentRouteChanged(newRoute, oldRoute);

    // Need to wait for currentRouteChanged observers on child views to run
    // first, before switching views.
    queueMicrotask(() => {
      switch (newRoute) {
        case routes.EXTENSIONS:
        case routes.BASIC:
          this.showDefaultViews_();
          break;
        case routes.EXTENSIONS_V2:
          this.$.viewManager.switchView(
            'manageProfile', 'no-animation', 'no-animation');
          break;
      }
    });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-extensions-page-index': SettingsBraveExtensionsPageIndexElement;
  }
}

customElements.define(
  SettingsBraveExtensionsPageIndexElement.is, SettingsBraveExtensionsPageIndexElement);
