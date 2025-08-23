// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CustomizeChromeApiProxy } from './customize_chrome_api_proxy.js'

import { CrLitElement, html, css } from '//resources/lit/v3_0/lit.rollup.js';
import { loadTimeData } from '//resources/js/load_time_data.js';
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'

import { AppElement } from './app-chromium.js';
export * from './app-chromium.js'

declare module './app-chromium.js' {
  interface AppElement {
    shouldShowDarkerThemeToggle_: boolean
  }
}

AppElement.prototype.shouldShowDarkerThemeToggle_ = loadTimeData.getBoolean('shouldShowDarkerThemeToggle');

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

// A component to toggle the "Darker theme" setting in the Customize Chrome side panel.
class DarkerThemeToggle extends I18nMixinLit(CrLitElement) {
  static get is() {
    return 'brave-darker-theme-toggle'
  }

  static override get properties() {
    return {
      usingDarkerTheme_: {type: Boolean},
    };
  }

  private accessor usingDarkerTheme_ = false

  static override get styles() {
    return css`
      #darker-theme-toggle-container {
        display: flex;
        align-items: center;
        gap: var(--leo-spacing-xl);
        margin-inline: var(--leo-spacing-xl);
      }

      #darker-theme-toggle-container > span {
        flex: 1;
      }`
  }

  override connectedCallback() {
    super.connectedCallback();

    if (!loadTimeData.getBoolean('shouldShowDarkerThemeToggle')) {
      return
    }

    const apiProxy = CustomizeChromeApiProxy.getInstance();

    apiProxy.callbackRouter.onUseDarkerThemeChanged.addListener((useDarkerTheme: boolean) => {
      this.usingDarkerTheme_ = useDarkerTheme;
    })

    apiProxy.handler.getUseDarkerTheme().then(({useDarkerTheme}) => {
      this.usingDarkerTheme_ = useDarkerTheme;
    });
  }

  override render() {
    return html`
        <div id="darker-theme-toggle-container">
          <leo-icon name="theme-darker"></leo-icon>
          <span>${this.i18n('CUSTOMIZE_CHROME_DARKER_THEME_TOGGLE_LABEL')}</span>
          <!-- Use cr-toggle instead of leo-toggle in order to inherit style -->
          <cr-toggle
            .checked="${this.usingDarkerTheme_}"
            @change="${this.onDarkerThemeToggleChange}"></cr-toggle>
        </div>
    `
  }

  private onDarkerThemeToggleChange() {
    CustomizeChromeApiProxy.getInstance().handler.setUseDarkerTheme(
      !this.usingDarkerTheme_
    )
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-darker-theme-toggle': DarkerThemeToggle;
  }
}

customElements.define(DarkerThemeToggle.is, DarkerThemeToggle);
