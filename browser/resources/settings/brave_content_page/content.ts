// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {DropdownMenuOptionList, SettingsDropdownMenuElement} from '../controls/settings_dropdown_menu.js';

import {BaseMixin} from '../base_mixin.js';
import {loadTimeData} from '../i18n_setup.js'
import {routes} from '../route.js';
import {Router} from '../router.js';

import {AppearanceBrowserProxy, AppearanceBrowserProxyImpl} from '../appearance_page/appearance_browser_proxy.js';
import {getTemplate} from './content.html.js'

/**
 * This is the absolute difference maintained between standard and
 * fixed-width font sizes. http://crbug.com/91922.
 */
const SIZE_DIFFERENCE_FIXED_STANDARD: number = 3;

export interface SettingsBraveContentContentElement {
  $: {
    defaultFontSize: SettingsDropdownMenuElement,
    zoomLevel: HTMLSelectElement,
  };
}

const SettingsBraveAppearanceContentElementBase =
    I18nMixin(PrefsMixin(BaseMixin(PolymerElement)));

export class SettingsBraveContentContentElement extends SettingsBraveAppearanceContentElementBase {
  static get is() {
    return 'settings-brave-content-content'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      defaultZoom_: Number,

      /**
       * List of options for the font size drop-down menu.
       */
      fontSizeOptions_: {
        readOnly: true,
        type: Array,
        value() {
          return [
            {value: 9, name: loadTimeData.getString('verySmall')},
            {value: 12, name: loadTimeData.getString('small')},
            {value: 16, name: loadTimeData.getString('medium')},
            {value: 20, name: loadTimeData.getString('large')},
            {value: 24, name: loadTimeData.getString('veryLarge')},
          ]
        },
      },

      /**
       * Predefined zoom factors to be used when zooming in/out. These are in
       * ascending order. Values are displayed in the page zoom drop-down menu
       * as percentages.
       */
      pageZoomLevels_: Array,
    }
  }

  static get observers() {
    return [
      'defaultFontSizeChanged_(prefs.webkit.webprefs.default_font_size.value)',
    ];
  }

  private fontSizeOptions_: DropdownMenuOptionList
  private pageZoomLevels_: number[]
  private defaultZoom_: number;
  private appearanceBrowserProxy_: AppearanceBrowserProxy =
      AppearanceBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()

    this.$.defaultFontSize.menuOptions = this.fontSizeOptions_;
    this.appearanceBrowserProxy_.getDefaultZoom().then(zoom => {
      this.defaultZoom_ = zoom;
    });

    this.pageZoomLevels_ =
        JSON.parse(loadTimeData.getString('presetZoomFactors'));
  }

  private onCustomizeFontsClick_() {
    Router.getInstance().navigateTo(routes.FONTS);
  }

  /**
   * @param value The changed font size slider value.
   */
  private defaultFontSizeChanged_(value: number) {
    // This pref is handled separately in some extensions, but here it is tied
    // to default_font_size (to simplify the UI).
    this.set(
        'prefs.webkit.webprefs.default_fixed_font_size.value',
        value - SIZE_DIFFERENCE_FIXED_STANDARD);
  }

  private onZoomLevelChange_() {
    chrome.settingsPrivate.setDefaultZoom(parseFloat(this.$.zoomLevel.value));
  }

  /** @see blink::PageZoomValuesEqual(). */
  private zoomValuesEqual_(zoom1: number, zoom2: number): boolean {
    return Math.abs(zoom1 - zoom2) <= 0.001;
  }

  /** @return A zoom easier read by users. */
  private formatZoom_(zoom: number): number {
    return Math.round(zoom * 100);
  }
}

customElements.define(SettingsBraveContentContentElement.is, SettingsBraveContentContentElement)
