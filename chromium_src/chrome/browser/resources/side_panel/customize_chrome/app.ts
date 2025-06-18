// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CustomizeChromeApiProxy } from './customize_chrome_api_proxy.js'

import { CrLitElement, html } from '//resources/lit/v3_0/lit.rollup.js';

export * from './app-chromium.js'

class ClosePanelButton extends CrLitElement {
  static get is() {
    return 'close-panel-button'
  }

  override render() {
    return html`
      <cr-icon-button
        id="closeButton"
        iron-icon="close"
        @click=${this.onClosePanel}
      ></cr-icon-button>
    `
  }

  private onClosePanel() {
    CustomizeChromeApiProxy.getInstance().handler.closePanel()
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'close-panel-button': ClosePanelButton
  }
}

customElements.define(ClosePanelButton.is, ClosePanelButton)
