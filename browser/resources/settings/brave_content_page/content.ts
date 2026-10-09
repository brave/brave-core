// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PrefService} from '/shared/settings/prefs2/pref_service.js';
import {PrefServiceObserverMixin} from '/shared/settings/prefs2/pref_service_observer_mixin.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {DropdownMenuOptionList, SettingsDropdownMenuElement} from '../controls/settings_dropdown_menu.js';

import {BaseMixin} from '../base_mixin.js';
import {loadTimeData} from '../i18n_setup.js'
import {routes} from '../route.js';
import {Router} from '../router.js';
import {SettingsViewMixin, SettingsViewMixinInterface} from '../settings_page/settings_view_mixin.js';

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
    I18nMixin(PrefServiceObserverMixin(BaseMixin(SettingsViewMixin(PolymerElement))));

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

      waybackMachineEnabledPref_: Object,

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

      showSplitViewDragAndDropSetting_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean('showSplitViewDragAndDropSetting');
        },
      },

      isWaybackMachineAutoCheckFeatureEnabled_: {
        type: Boolean,
        value() {
          return loadTimeData.getBoolean(
              'isWaybackMachineAutoCheckFeatureEnabled');
        },
      },

    }
  }

  private declare fontSizeOptions_: DropdownMenuOptionList
  private declare pageZoomLevels_: number[]
  private declare defaultZoom_: number;
  private declare waybackMachineEnabledPref_:
      chrome.settingsPrivate.PrefObject<boolean>|undefined;
  private declare showSplitViewDragAndDropSetting_: boolean;
  private declare isWaybackMachineAutoCheckFeatureEnabled_: boolean;
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

  override connectedCallback() {
    super.connectedCallback();
    this.addPrefObserver<number>(
        'webkit.webprefs.default_font_size',
        pref => this.defaultFontSizeChanged_(pref.value));
    this.mirrorPref(
        'brave.wayback_machine_enabled', 'waybackMachineEnabledPref_');
  }

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'fonts':
        return this.shadowRoot!.querySelector('#customize-fonts-subpage-trigger')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
  }

  private onCustomizeFontsClick_() {
    Router.getInstance().navigateTo(routes.FONTS);
  }

  private isWaybackMachineManaged_(
      pref: chrome.settingsPrivate.PrefObject): boolean {
    return pref &&
        pref.enforcement === chrome.settingsPrivate.Enforcement.ENFORCED;
  }

  private shouldShowWaybackMachineAutoCheck_(
      featureEnabled: boolean, waybackEnabled: boolean): boolean {
    return featureEnabled && waybackEnabled;
  }

  /**
   * @param value The changed font size slider value.
   */
  private defaultFontSizeChanged_(value: number) {
    // This pref is handled separately in some extensions, but here it is tied
    // to default_font_size (to simplify the UI).
    PrefService.getInstance().setPrefValue(
        'webkit.webprefs.default_fixed_font_size',
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
