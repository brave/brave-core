/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {SettingsPrivacyPageElement} from '../privacy_page/privacy_page.js'

// Extend SettingsPrivacyPageElement to add clear cookies handler.
export class BraveSettingsPrivacyPageElement
    extends SettingsPrivacyPageElement {
  onRemoveAllCookiesFromSite_ () {
    // Intentionally not casting to SiteDataDetailsSubpageElement, as this would
    // require importing site_data_details_subpage.js and would endup in the
    // main JS bundle.
    const node =
      this.shadowRoot!.querySelector('brave-site-data-details-subpage');
    if (node) {
      node.removeAll();
    }
  }
}
