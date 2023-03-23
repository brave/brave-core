/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {SiteDetailsElement} from '../site_settings/site_details.js'
import {Router} from '../router.js';
import {routes} from '../route.js';

// Extend SiteDetailsElement to add navigation to cookies subpage.
export class BraveSiteDetailsElement extends SiteDetailsElement {
  onCookiesDetailClicked_ () {
    const site = Router.getInstance().getQueryParameters().get('site');
    if (!site) {
      return;
    }
    const searchParams = new URLSearchParams(
      'site=' +
      (new URL(site)).host);
    Router.getInstance().navigateTo(routes.BRAVE_SITE_SETTINGS_COOKIES_DETAILS,
                                    searchParams);
  }
}
