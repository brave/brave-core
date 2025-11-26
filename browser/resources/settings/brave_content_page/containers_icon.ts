// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { Icon } from '../containers.mojom-webui.js'
import { getCss } from './containers_icon.css.js'
import { getHtml } from './containers_icon.html.js'

export type IconSelectedEvent = CustomEvent<{
  icon: Icon
}>

const iconsMap = new Map<Icon, string>([
  [Icon.kPersonal, 'container-personal'],
  [Icon.kWork, 'container-work'],
  [Icon.kShopping, 'container-shopping'],
  [Icon.kSocial, 'container-social'],
  [Icon.kEvents, 'container-events'],
  [Icon.kBanking, 'container-banking'],
  [Icon.kStar, 'container-star'],
  [Icon.kTravel, 'container-travel'],
  [Icon.kSchool, 'container-school'],
  [Icon.kPrivate, 'container-private'],
  [Icon.kMessaging, 'container-messaging'],
])

export class SettingsBraveContentContainersIconElement extends CrLitElement {
  static get is() {
    return 'settings-brave-content-containers-icon'
  }

  static override get properties() {
    return {
      icon: { type: Number },
      backgroundColor: { type: String },
      selected: { type: Boolean },
    }
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  accessor icon: Icon
  get leoIcon() {
    const icon = iconsMap.get(this.icon)
    if (!icon) {
      console.warn(`No Leo icon found for icon: ${this.icon} from `, iconsMap)
      return ''
    }
    return icon
  }

  accessor backgroundColor: string = 'magenta' // Debug color that should never be visible
  accessor selected: boolean = false

  handleIconClick_() {
    const event: IconSelectedEvent = new CustomEvent('icon-selected', {
      bubbles: true,
      composed: true,
      detail: { icon: this.icon },
    })

    this.dispatchEvent(event)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-containers-icon': SettingsBraveContentContainersIconElement
  }
}

customElements.define(
  SettingsBraveContentContainersIconElement.is,
  SettingsBraveContentContainersIconElement,
)
