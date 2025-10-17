// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import '/shared/settings/prefs/prefs.js';

import '../settings_shared.css.js';

import type { CrViewManagerElement } from 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import { RouteObserverMixin } from '../router.js';
import type { Route } from '../router.js';
import { routes } from '../route.js';

import type { SettingsPlugin } from '../settings_main/settings_plugin.js';
import { SearchableViewContainerMixin } from '../settings_page/searchable_view_container_mixin.js';

import { getTemplate } from './content_page_index.html.js';
import {loadTimeData} from '../i18n_setup.js'

import {pageVisibility} from '../page_visibility.js';
import type {PageVisibility} from '../page_visibility.js';

export interface SettingsBraveContentPageIndexElement {
  $: {
    viewManager: CrViewManagerElement,
  };
}

const SettingsBraveContentPageIndexElementBase =
  SearchableViewContainerMixin(RouteObserverMixin(PolymerElement));

export class SettingsBraveContentPageIndexElement extends
SettingsBraveContentPageIndexElementBase implements SettingsPlugin {
  static get is() {
    return 'settings-brave-content-page-index';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      prefs: Object,

      pageVisibility_: {
        type: Object,
        value() {
          return pageVisibility || {};
        },
      },
    };
  }

  declare prefs: { [key: string]: any };
  declare private pageVisibility_: PageVisibility;

  private showDefaultViews_() {
    const views = ['parent'];
    if (this.pageVisibility_.containers) {
      views.push('containers');
    }
    if (this.showPage_(this.pageVisibility_.playlist)) {
      views.push('playlist');
    }
    // <if expr="enable_speedreader">
    if (this.showPage_(this.pageVisibility_.speedreader)) {
      views.push('speedreader');
    }
    // </if>
    this.$.viewManager.switchViews(views, 'no-animation', 'no-animation');
  }

  private showPage_(visibility?: boolean): boolean {
    return visibility !== false;
  }

  override currentRouteChanged(newRoute: Route, oldRoute?: Route) {
    super.currentRouteChanged(newRoute, oldRoute);

    // Need to wait for currentRouteChanged observers on child views to run
    // first, before switching views.
    queueMicrotask(() => {
      switch (newRoute) {
        case routes.BRAVE_CONTENT:
        case routes.BASIC:
          this.showDefaultViews_();
          break;
        case routes.FONTS:
          this.$.viewManager.switchView('fonts', 'no-animation', 'no-animation');
          break;
      }
    });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-page-index': SettingsBraveContentPageIndexElement;
  }
}

customElements.define(
  SettingsBraveContentPageIndexElement.is, SettingsBraveContentPageIndexElement);
