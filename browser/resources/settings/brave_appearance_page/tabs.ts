// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PrefServiceObserverMixin, PrefServiceObserverMixinInterface} from '/shared/settings/prefs2/pref_service_observer_mixin.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {OpenWindowProxyImpl} from 'chrome://resources/js/open_window_proxy.js';
import {sendWithPromise} from 'chrome://resources/js/cr.js'

import {loadTimeData} from '../i18n_setup.js'

import {getTemplate} from './tabs.html.js'

const SettingsBraveAppearanceTabsElementBase =
    WebUiListenerMixin(PrefServiceObserverMixin(I18nMixin(PolymerElement))) as {
  new (): PolymerElement & I18nMixinInterface &
      WebUiListenerMixinInterface & PrefServiceObserverMixinInterface
}

type PrefObject<T> = chrome.settingsPrivate.PrefObject<T>

export class SettingsBraveAppearanceTabsElement extends SettingsBraveAppearanceTabsElementBase {
  static get is() {
    return 'settings-brave-appearance-tabs'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      tabMinWidthModes_: {
        readOnly: true,
        type: Array,
        value() {
          return [
            {
              value: 0,
              hidden: true,
            },
            {
              value: 1,
              name: loadTimeData.getString('appearanceSettingsTabMinWidthMinimum'),
            },
            {
              value: 2,
              name: loadTimeData.getString('appearanceSettingsTabMinWidthMedium'),
            },
            {
              value: 3,
              name: loadTimeData.getString('appearanceSettingsTabMinWidthLarge'),
            },
            {
              value: 4,
              name: loadTimeData.getString('appearanceSettingsTabMinWidthFull'),
            },
          ]
        },
      },
      tabTooltipModes_: {
        readyOnly: true,
        type: Array,
        value() {
          return [
            {
              value: 1,
              name: loadTimeData.getString('appearanceSettingsTabHoverModeCard')
            },
            {
              value: 2,
              name: loadTimeData.getString(
                'appearanceSettingsTabHoverModeCardWithPreview')
            },
            {
              value: 0,
              name: loadTimeData.getString('appearanceSettingsTabHoverModeTooltip')
            }
          ]
        }
      },
      verticalTabsToggleEnabled_: {
        type: Boolean,
        value: true,
      },

      // Mirrored from the global PrefService, purely to evaluate dom-if
      // conditions and computed bindings in this element's own template. The
      // controls themselves read/write via `pref-key` directly.
      verticalTabsEnabledPref_: Object,
      verticalTabsHideCompletelyWhenCollapsedPref_: Object,
      verticalTabsShowToggleButtonPref_: Object,
      scrollableHorizontalTabStripPref_: Object,
    }
  }

  declare private tabMinWidthModes_: Array<{
    value: number,
    name: string,
    hidden?: boolean,
  }>
  declare private tabTooltipModes_:
      Array<{value: number, name: string}>
  declare private verticalTabsToggleEnabled_: boolean
  declare private verticalTabsEnabledPref_: PrefObject<boolean>|undefined
  declare private verticalTabsHideCompletelyWhenCollapsedPref_:
      PrefObject<boolean>|undefined
  declare private verticalTabsShowToggleButtonPref_: PrefObject<boolean>|undefined
  declare private scrollableHorizontalTabStripPref_: PrefObject<boolean>|undefined

  override connectedCallback() {
    super.connectedCallback()
    sendWithPromise<boolean>('getIsVerticalTabsToggleEnabled').then(
        (enabled: boolean) => { this.verticalTabsToggleEnabled_ = enabled })
    this.addWebUiListener(
        'vertical-tabs-toggle-enabled-changed',
        (enabled: boolean) => { this.verticalTabsToggleEnabled_ = enabled })

    this.mirrorPrefs({
      'brave.tabs.vertical_tabs_enabled': 'verticalTabsEnabledPref_',
      'brave.tabs.vertical_tabs_hide_completely_when_collapsed':
          'verticalTabsHideCompletelyWhenCollapsedPref_',
      'brave.tabs.vertical_tabs_show_toggle_button':
          'verticalTabsShowToggleButtonPref_',
      'brave.tabs.scrollable_horizontal_tab_strip':
          'scrollableHorizontalTabStripPref_',
    })
  }

  private isSharedPinnedTabsEnabled_() {
    return loadTimeData.getBoolean('isSharedPinnedTabsEnabled')
  }

  private onDiscardRingTreatmentLearnMoreLinkClick_() {
    OpenWindowProxyImpl.getInstance().openUrl(
      loadTimeData.getString('discardRingTreatmentLearnMoreUrl'));
  }

  private isTreeTabsFlagEnabled() {
    return loadTimeData.getBoolean('isTreeTabsFlagEnabled');
  }

  private isHideVerticalTabCompletelyFlagEnabled() {
    return loadTimeData.getBoolean('isHideVerticalTabCompletelyFlagEnabled');
  }

  // "Float on mouse over" (auto-expand on hover) is forced on, and its
  // checkbox shown as checked and disabled, when there is no other way for
  // the user to expand collapsed vertical tabs: either because the vertical
  // tab strip is fully hidden when collapsed, or because the toggle button
  // used to expand/collapse it is hidden.
  private shouldForceFloatOnMouseOver_(
      hideCompletelyWhenCollapsed: boolean, showToggleButton: boolean) {
    return (this.isHideVerticalTabCompletelyFlagEnabled() &&
            hideCompletelyWhenCollapsed) ||
        !showToggleButton
  }

  private isScrollableHorizontalTabStripFlagEnabled() {
    return loadTimeData.getBoolean('isScrollableHorizontalTabStripEnabled');
  }
}

customElements.define(SettingsBraveAppearanceTabsElement.is, SettingsBraveAppearanceTabsElement)
