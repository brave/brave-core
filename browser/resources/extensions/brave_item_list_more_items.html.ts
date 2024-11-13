// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { html } from '//resources/lit/v3_0/lit.rollup.js';
import type { ExtensionsBraveItemListMoreItemsElement } from './brave_item_list_more_items.js'

export function getHtml(this: ExtensionsBraveItemListMoreItemsElement) {
  return html`<div id="more-items" class="more-items-message"
     hidden$="${!this.shouldShowMoreItemsMessage_()}">
  <span>
    $i18nRaw{noExtensionsOrApps}
  </span>
</div>`
}
