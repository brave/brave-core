/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Based on Chromium code subject to the following license:
// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_elements/cr_expand_button/cr_expand_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';
import 'chrome://resources/cr_elements/cr_shared_style.css.js';
import 'chrome://resources/polymer/v3_0/iron-collapse/iron-collapse.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import '../settings_shared.css.js';

import '//resources/js/cr.js';

import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {loadTimeData} from '../i18n_setup.js';
import {MetricsBrowserProxyImpl, PrivacyElementInteractions} from '../metrics_browser_proxy.js';
import {routes} from '../route.js';
import {Route, RouteObserverMixin, RouteObserverMixinInterface, Router} from '../router.js';

import {CookieDataForDisplay, CookieDetails, getCookieData} from './cookie_info.js';
import {LocalDataBrowserProxy, LocalDataBrowserProxyImpl} from './local_data_browser_proxy.js';
import {getTemplate} from './brave_site_data_details_subpage.html.js';


const categoryLabels: {[key: string]: string} = {
  cache_storage: loadTimeData.getString('cookieCacheStorage'),
  database: loadTimeData.getString('cookieDatabaseStorage'),
  file_system: loadTimeData.getString('cookieFileSystem'),
  flash_lso: loadTimeData.getString('cookieFlashLso'),
  indexed_db: loadTimeData.getString('cookieDatabaseStorage'),
  local_storage: loadTimeData.getString('cookieLocalStorage'),
  service_worker: loadTimeData.getString('cookieServiceWorker'),
  shared_worker: loadTimeData.getString('cookieSharedWorker'),
  quota: loadTimeData.getString('cookieQuotaStorage'),
};

/**
 * 'brave-site-data-details-subpage' Display cookie contents.
 * Restore removed from Chromium site data details subpage
 * https://chromium.googlesource.com/chromium/src/+/
 * 22ecdb41cdc603d40b47f92addcd52933fdef445/chrome/browser/resources/settings/
 * site_settings/site_data_details_subpage.js
 */

const SiteDataDetailsSubpageElementBase =
    RouteObserverMixin(WebUiListenerMixin(PolymerElement)) as {
      new (): PolymerElement & WebUiListenerMixinInterface &
          RouteObserverMixinInterface,
    };

export class BraveSiteDataDetailsSubpageElement extends
    SiteDataDetailsSubpageElementBase {
  static get is() {
    return 'brave-site-data-details-subpage';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /**
       * The cookie entries for the given site.
       */
      entries_: Array,

      /** Set the page title on the settings-subpage parent. */
      pageTitle: {
        type: String,
        notify: true,
      },

      site_: String,
    };
  }

  private entries_: Array<CookieDetails&{expanded_: boolean}>;
  pageTitle: string;
  private site_: string;
  private browserProxy_: LocalDataBrowserProxy =
      LocalDataBrowserProxyImpl.getInstance();

  override ready() {
    super.ready();
    this.addWebUiListener(
        'on-tree-item-removed', () => this.getCookieDetails_());
  }

  /**
   * RouteObserverMixin
   */
  override currentRouteChanged(route: Route) {
    if (route !== routes.BRAVE_SITE_SETTINGS_COOKIES_DETAILS) {
      return;
    }
    const site = Router.getInstance().getQueryParameters().get('site');
    if (!site) {
      return;
    }
    this.site_ = site;
    this.pageTitle = loadTimeData.getStringF('siteSettingsCookieSubpage', site);
    this.getCookieDetails_();
  }

  private getCookieDetails_() {
    if (!this.site_) {
      return;
    }
    this.browserProxy_.getCookieDetails(this.site_)
        .then(
            (cookies: CookieDetails[]) => this.onCookiesLoaded_(cookies),
            () => this.onCookiesLoadFailed_());
  }

  private getCookieNodes_(node: CookieDetails): CookieDataForDisplay[] {
    return getCookieData(node);
  }

  private onCookiesLoaded_(cookies: CookieDetails[]) {
    this.entries_ = cookies.map(c => {
      // Set up flag for expanding cookie details.
      (c as CookieDetails & {expanded_: boolean}).expanded_ = false;
      return c;
    }) as Array<CookieDetails&{expanded_: boolean}>;
  }

  /**
   * The site was not found. E.g. The site data may have been deleted or the
   * site URL parameter may be mistyped.
   */
  private onCookiesLoadFailed_() {
    this.entries_ = [];
  }

  /**
   * Retrieves a string description for the provided |item|.
   */
  private getEntryDescription_(item: CookieDetails): string {
    // Frequently there are multiple cookies per site. To avoid showing a list
    // of '1 cookie', '1 cookie', ... etc, it is better to show the title of the
    // cookie to differentiate them.
    if (item.type === 'cookie') {
      return item.title;
    }
    return categoryLabels[item.type];
  }

  /**
   * A handler for when the user opts to remove a single cookie.
   */
  private onRemove_(event: Event) {
    this.browserProxy_.removeItem(
        (event.currentTarget as HTMLElement).dataset['idPath']!);
  }

  /**
   * A handler for when the user opts to remove all cookies.
   */
  removeAll() {
    MetricsBrowserProxyImpl.getInstance().recordSettingsPageHistogram(
        PrivacyElementInteractions.COOKIE_DETAILS_REMOVE_ALL);
    this.browserProxy_.removeSite(this.site_);
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-site-data-details-subpage': BraveSiteDataDetailsSubpageElement;
  }
}

customElements.define(
  BraveSiteDataDetailsSubpageElement.is, BraveSiteDataDetailsSubpageElement);
