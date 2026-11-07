/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as SDK from '../../core/sdk/sdk.js'
import * as Lit from '../../ui/lit/lit.js'
import * as UI from '../../ui/legacy/legacy.js'

const { render, html } = Lit

export class RequestAdblockView extends UI.Widget.VBox {
  readonly #request: SDK.NetworkRequest.NetworkRequest
  #manager: SDK.NetworkManager.NetworkManager | null

  constructor(request: SDK.NetworkRequest.NetworkRequest) {
    super()
    this.#request = request
    this.#manager = SDK.NetworkManager.NetworkManager.forRequest(this.#request)
  }

  override performUpdate(): void {
    if (!this.#manager) {
      render(html``, this.contentElement, { host: this })
      return
    }
    const adblockInfo = (this.#manager.dispatcher as any).adblockInfoForId(
      this.#request.requestId()
    )
    render(
      html` ${Object.entries(adblockInfo ? adblockInfo : {}).map(
        ([key, value]) => {
          const displayValue =
            value && typeof value === 'object'
              ? JSON.stringify(value)
              : `${value}`
          return this.#renderRow(key, displayValue)
        }
      )}`,
      this.contentElement,
      { host: this }
    )
  }

  #renderRow(title: string, value: string): Lit.LitTemplate {
    const rowStyle = {
      'display': 'flex',
      'line-height': '20px',
      'padding-left': '8px',
      'gap': '12px',
      'user-select': 'text',
      'font-weight': '400'
    }

    const titleStyle = {
      'width': '30%',
      'min-width': '160px',
      'max-width': '240px',
      'flex-shrink': '0',
      'text-transform': 'capitalize',
      'background': 'transparent'
    }

    return html`
      <div style=${Lit.Directives.styleMap(rowStyle)}>
        <div style=${Lit.Directives.styleMap(titleStyle)}>${title}</div>
        <div>${value}</div>
      </div>
    `
  }

  #refresh(): void {
    this.requestUpdate()
  }

  override wasShown(): void {
    super.wasShown()
    this.#request.addEventListener(
      SDK.NetworkRequest.Events.FINISHED_LOADING,
      this.#refresh,
      this
    )
  }

  override willHide(): void {
    super.willHide()
    this.#request.removeEventListener(
      SDK.NetworkRequest.Events.FINISHED_LOADING,
      this.#refresh,
      this
    )
  }
}
