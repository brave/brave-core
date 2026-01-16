// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CustomizeChromeApiProxy } from './customize_chrome_api_proxy.js'

import {
  css,
  html,
  CrLitElement,
  type CSSResultGroup
} from '//resources/lit/v3_0/lit.rollup.js'
import { loadTimeData } from '//resources/js/load_time_data.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
import { ColorSchemeMode } from '//resources/cr_components/customize_color_scheme_mode/customize_color_scheme_mode.mojom-webui.js'
import { CustomizeColorSchemeModeBrowserProxy } from '//resources/cr_components/customize_color_scheme_mode/browser_proxy.js'

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

// A component to toggle the "Darker theme" setting in the Customize Chrome side panel.
class DarkerThemeToggle extends I18nMixinLit(CrLitElement) {
  static get is() {
    return 'brave-darker-theme-toggle'
  }

  static override get properties() {
    return {
      shouldShowDarkerThemeToggle_: { type: Boolean },
      usingDarkerTheme_: { type: Boolean },
    }
  }

  private setColorSchemeModeListenerId_: number | null = null

  private accessor shouldShowDarkerThemeToggle_: boolean = false

  private accessor usingDarkerTheme_ = false

  static override get styles(): CSSResultGroup {
    return css`
      #darker-theme-toggle-container {
        display: flex;
        align-items: center;
        gap: var(--leo-spacing-xl);
        margin-inline: var(--leo-spacing-xl);
      }

      #darker-theme-toggle-container[hidden='true'] {
        display: none;
      }

      #darker-theme-toggle-container > span {
        flex: 1;
      }
    `
  }

  override connectedCallback() {
    super.connectedCallback()

    if (!loadTimeData.getBoolean('shouldShowDarkerThemeToggle')) {
      return
    }

    const apiProxy = CustomizeChromeApiProxy.getInstance()

    apiProxy.callbackRouter.onUseDarkerThemeChanged.addListener(
      (useDarkerTheme: boolean) => {
        this.usingDarkerTheme_ = useDarkerTheme
      },
    )

    apiProxy.handler.getUseDarkerTheme().then(({ useDarkerTheme }) => {
      this.usingDarkerTheme_ = useDarkerTheme
    })

    const colorSchemeModeClientCallbackRouter =
      CustomizeColorSchemeModeBrowserProxy.getInstance().callbackRouter
    this.setColorSchemeModeListenerId_ =
      colorSchemeModeClientCallbackRouter.setColorSchemeMode.addListener(
        (colorSchemeMode: ColorSchemeMode) => {
          this.shouldShowDarkerThemeToggle_ =
            colorSchemeMode !== ColorSchemeMode.kLight
        },
      )
  }

  override disconnectedCallback() {
    if (this.setColorSchemeModeListenerId_) {
      const colorSchemeModeClientCallbackRouter =
        CustomizeColorSchemeModeBrowserProxy.getInstance().callbackRouter
      colorSchemeModeClientCallbackRouter.removeListener(
        this.setColorSchemeModeListenerId_,
      )
      this.setColorSchemeModeListenerId_ = null
    }
    super.disconnectedCallback()
  }

  override render() {
    return html`
      <div
        id="darker-theme-toggle-container"
        hidden="${!this.shouldShowDarkerThemeToggle_}"
      >
        <leo-icon name="theme-darker"></leo-icon>
        <span>${this.i18n('CUSTOMIZE_CHROME_DARKER_THEME_TOGGLE_LABEL')}</span>
        <!-- Use cr-toggle instead of leo-toggle in order to inherit style -->
        <cr-toggle
          .checked="${this.usingDarkerTheme_}"
          @change="${this.onDarkerThemeToggleChange}"
        ></cr-toggle>
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
    'brave-darker-theme-toggle': DarkerThemeToggle
  }
}

customElements.define(DarkerThemeToggle.is, DarkerThemeToggle)
