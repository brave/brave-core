/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import 'chrome://resources/cr_elements/cr_shared_style.css.js'
import 'chrome://resources/js/cr.js'
import {getTemplate} from './brave_item_list_more_items.html.js'

const ExtensionsBraveItemListMoreItemsElementBase =
  I18nMixin(PolymerElement) as {
    new(): PolymerElement & I18nMixinInterface
  }

export class ExtensionsBraveItemListMoreItemsElement extends ExtensionsBraveItemListMoreItemsElementBase {
  static get is() {
    return 'extensions-brave-item-list-more-items'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /** @type {!Array<!chrome.developerPrivate.ExtensionInfo>} */
      apps: Array,

      /** @type {!Array<!chrome.developerPrivate.ExtensionInfo>} */
      extensions: Array,
    };
  }

  private apps: chrome.developerPrivate.ExtensionInfo[];
  private extensions: chrome.developerPrivate.ExtensionInfo[];

  shouldShowMoreItemsMessage_() {
    if (!this.apps || !this.extensions)
      return;

    return this.apps.length !== 0 || this.extensions.length !== 0;
  }
}

customElements.define(
  ExtensionsBraveItemListMoreItemsElement.is, ExtensionsBraveItemListMoreItemsElement)

