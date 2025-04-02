/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './strings.m.js'

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

import { getCss } from './hello_world_app.css.js'
import { getHtml } from './hello_world_app.html.js'

export class HelloWorldElement extends CrLitElement {
  static get is() {
    return 'hello-world'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      platform: { type: String },
    }
  }

  protected platform: string = loadTimeData.getString('platform')
}

declare global {
  interface HTMLElementTagNameMap {
    'hello-world': HelloWorldElement
  }
}

customElements.define(HelloWorldElement.is, HelloWorldElement)
