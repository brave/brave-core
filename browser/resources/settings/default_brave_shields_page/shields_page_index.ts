// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import '/shared/settings/prefs/prefs.js';
import '../settings_shared.css.js';

import type {CrViewManagerElement} from 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {routes} from '../route.js';
import {RouteObserverMixin} from '../router.js';
import type {Route} from '../router.js';
import type {SettingsPlugin} from '../settings_main/settings_plugin.js';
import {SearchableViewContainerMixin} from '../settings_page/searchable_view_container_mixin.js';

import {getTemplate} from './shields_page_index.html.js';

// Subpages
import './default_brave_shields_page.js';
import './brave_adblock_subpage.js';


export interface SettingsShieldsPageIndexElement {
  $: {
    viewManager: CrViewManagerElement,
  };
}

const SettingsShieldsPageIndexElementBase =
    SearchableViewContainerMixin(RouteObserverMixin(PolymerElement));

export class SettingsShieldsPageIndexElement extends
    SettingsShieldsPageIndexElementBase implements SettingsPlugin {
  static get is() {
    return 'settings-shields-page-index';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      prefs: Object,
    };
  }

  declare prefs: {[key: string]: any};

  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    super.currentRouteChanged(newRoute, oldRoute);

    // Need to wait for currentRouteChanged observers on child views to run
    // first, before switching views.
    queueMicrotask(() => {
      switch (newRoute) {
        case routes.SHIELDS:
          this.$.viewManager.switchView(
              'parent', 'no-animation', 'no-animation');
          break;
        case routes.SHIELDS_ADBLOCK:
          this.$.viewManager.switchView(
              'adblock', 'no-animation', 'no-animation');
          break;
        case routes.SOCIAL_BLOCKING:
          this.$.viewManager.switchView(
              'socialBlocking', 'no-animation', 'no-animation');
          break;
        case routes.BASIC:
          // Switch back to the default view in case they are part of search
          // results.
          this.$.viewManager.switchView(
              'parent', 'no-animation', 'no-animation');
          break;
        default:
          // Nothing to do. Other parent elements are responsible for updating
          // the displayed contents.
          break;
      }
    });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-shields-page-index': SettingsShieldsPageIndexElement;
  }
}

customElements.define(
  SettingsShieldsPageIndexElement.is, SettingsShieldsPageIndexElement);
