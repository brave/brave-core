// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { hexColorToSkColor } from '//resources/js/color_utils.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { SkColor } from '//resources/mojo/skia/public/mojom/skcolor.mojom-webui.js'

import { getCss } from './containers_background_chip.css.js'
import { getHtml } from './containers_background_chip.html.js'

export type ColorSelectedEvent = CustomEvent<{ color: SkColor }>

export class SettingsBraveContentContainersBackgroundChipElement extends CrLitElement {
  static get is() {
    return 'settings-brave-content-containers-background-chip'
  }

  static override get properties() {
    return {
      selected: { type: Boolean },
      backgroundColor: { type: String },
    }
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  accessor selected: boolean = false
  accessor backgroundColor: string = 'magenta' // Debug color that should never be visible

  handleChipClick_() {
    const event: ColorSelectedEvent = new CustomEvent('background-selected', {
      bubbles: true,
      composed: true,
      detail: { color: hexColorToSkColor(this.backgroundColor) },
    })
    this.dispatchEvent(event)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-containers-background-chip': SettingsBraveContentContainersBackgroundChipElement
  }
}

customElements.define(
  SettingsBraveContentContainersBackgroundChipElement.is,
  SettingsBraveContentContainersBackgroundChipElement,
)
