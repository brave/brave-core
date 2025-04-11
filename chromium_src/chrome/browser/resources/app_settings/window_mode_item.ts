// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { App } from 'chrome://resources/cr_components/app_management/app_management.mojom-webui.js';
import { CrLitElement, css } from 'chrome://resources/lit/v3_0/lit.rollup.js';

import { getCss } from './app_management_shared_style.css.js';
import { createDummyApp } from './web_app_settings_utils.js';
import { getHtml } from './window_mode_item.html.js';

export class WindowModeItemElement extends CrLitElement {
  static get is() {
    return 'app-management-window-mode-item';
  }

  static override get styles() {
    return [
      getCss(),
      css`
        :host > div {
          width: 100%;
        }

        leo-radiobutton {
          display: flex;
        }
      `
    ];
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      windowModeLabel: { type: String },

      app: { type: Object },
    };
  }

  windowModeLabel: string = '';
  app: App = createDummyApp();
}

declare global {
  interface HTMLElementTagNameMap {
    'app-management-window-mode-item': WindowModeItemElement;
  }
}

customElements.define(WindowModeItemElement.is, WindowModeItemElement);
