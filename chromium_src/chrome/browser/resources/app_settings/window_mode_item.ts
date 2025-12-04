// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { App } from '//resources/cr_components/app_management/app_management.mojom-webui.js';
import { CrLitElement, css } from '//resources/lit/v3_0/lit.rollup.js';

import { getCss } from './app_management_shared_style.css.js';
import { createDummyApp } from './web_app_settings_utils.js';
import { getHtml } from './window_mode_item.html.js';
import { loadTimeData } from '//resources/js/load_time_data.js';
import { BrowserProxy } from '//resources/cr_components/app_management/browser_proxy.js';
import {WindowMode} from '//resources/cr_components/app_management/app_management.mojom-webui.js';

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

  get tabStripSettingsEnabled_(): boolean {
    return loadTimeData.getBoolean('isPWAsTabStripSettingsEnabled');
  }

  get windowModes() {
    return [
      {
        label: loadTimeData.getString('appManagementWindowModeLabel'),
        value: WindowMode.kWindow,
      },
      {
        label: loadTimeData.getString('appManagementBrowserModeLabel'),
        value: WindowMode.kBrowser,
      },
      ...this.tabStripSettingsEnabled_ ? [{
        label: loadTimeData.getString('appManagementTabbedWindowModeLabel'),
        value: WindowMode.kTabbedWindow,
      }] : [],
    ]
  }

  accessor windowModeLabel: string = '';
  accessor app: App = createDummyApp();

  onChange(e: { value: string }) {
    BrowserProxy.getInstance().handler.setWindowMode(this.app.id, parseInt(e.value) as WindowMode);
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'app-management-window-mode-item': WindowModeItemElement;
  }
}

customElements.define(WindowModeItemElement.is, WindowModeItemElement);
