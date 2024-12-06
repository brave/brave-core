/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit, I18nMixinLitInterface } from '//resources/cr_elements/i18n_mixin_lit.js'
import '//resources/js/cr.js'

import {getCss} from './brave_item_list_more_items.css.js'
import { getHtml } from './brave_item_list_more_items.html.js'

const ExtensionsBraveItemListMoreItemsElementBase =
  I18nMixinLit(CrLitElement) as {
    new(): CrLitElement & I18nMixinLitInterface
  }

export class ExtensionsBraveItemListMoreItemsElement extends ExtensionsBraveItemListMoreItemsElementBase {
  static get is() {
    return 'extensions-brave-item-list-more-items'
  }

  // @ts-ignore
  static override get styles() {
    return getCss()
  }

  // @ts-ignore
  override render() {
    return getHtml.bind(this)()
  }

  // @ts-ignore
  static override get properties() {
    return {
      apps: { type: Array },
      extensions: { type: Array },
    };
  }

  private apps: chrome.developerPrivate.ExtensionInfo[] = [];
  private extensions: chrome.developerPrivate.ExtensionInfo[] = [];

  shouldShowMoreItemsMessage_() {
    if (!this.apps || !this.extensions)
      return;

    return this.apps.length !== 0 || this.extensions.length !== 0;
  }
}

customElements.define(
  ExtensionsBraveItemListMoreItemsElement.is, ExtensionsBraveItemListMoreItemsElement)

