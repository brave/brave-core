/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/brave/leo.bundle.js'

import {CrLitElement} from
  '//resources/lit/v3_0/lit.rollup.js'

import {
  ModelWithSubtitle,
  ModelAccess,
} from './brave_leo_assistant_browser_proxy.js'
import {getCss} from './model_selector.css.js'
import {getHtml} from './model_selector.html.js'

export class LeoModelSelectorElement extends CrLitElement {
  static get is() {
    return 'leo-model-selector'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      selectedKey: {type: String},
      models: {type: Array},
      isPremiumUser: {type: Boolean},
    }
  }

  accessor selectedKey: string = ''
  accessor models: ModelWithSubtitle[] = []
  accessor isPremiumUser: boolean = false

  get selectedDisplayName(): string {
    return this.models?.find(
      (entry) => entry.model.key === this.selectedKey
    )?.model.displayName ?? ''
  }

  onSelectionChange_(e: any) {
    this.fire('model-changed', {value: e.value})
  }

  shouldShowPremiumLabel_(entry: ModelWithSubtitle):
      boolean {
    if (!entry.model.options.leoModelOptions) {
      return false
    }
    return entry.model.options.leoModelOptions.access
        === ModelAccess.PREMIUM && !this.isPremiumUser
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'leo-model-selector': LeoModelSelectorElement
  }
}

customElements.define(
  LeoModelSelectorElement.is, LeoModelSelectorElement)
