// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js'
import '../settings_vars.css.js'
import 'chrome://resources/cr_elements/cr_button/cr_button.js'

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

// Mirrors BraveAppearanceHandler::VerticalTabsWindowState().
type VerticalTabsWindowState = {
  enabled: boolean,
  hasOverride: boolean,
}

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

      // Mirrored from the global PrefService for dom-if conditions.
      verticalTabsEnabledPref_: Object,
      verticalTabsEnabledForWindow_: {
        type: Boolean,
        value: false,
      },
      // True while this window's layout disagrees with the shared switch.
      // A choice that matches the switch is cleared and is not an override.
      hasVerticalTabsWindowOverride_: {
        type: Boolean,
        value: false,
      },
      verticalTabsHideCompletelyWhenCollapsedPref_: Object,
      verticalTabsShowToggleButtonPref_: Object,
      scrollableHorizontalTabStripPref_: Object,

      // Resolved once at page load: which pref backs the "float on mouse
      // over" control depends on whether upstream's vertical tabs feature
      // (rather than Brave's own implementation) is the one actually live.
      // See VerticalTabController::SupportsBraveVerticalTabs().
      floatingModePrefKey_: {
        type: String,
        readOnly: true,
        value() {
          return loadTimeData.getBoolean('isUpstreamVerticalTabsFeatureEnabled')
              ? 'vertical_tabs.expand_on_hover'
              : 'brave.tabs.vertical_tabs_floating_enabled'
        },
      },

      // Resolved once at page load: which pref backs the "Use vertical tabs"
      // toggle depends on whether upstream's vertical tabs feature (rather than
      // Brave's own implementation) is the one actually live.
      verticalTabsEnabledPrefKey_: {
        type: String,
        readOnly: true,
        value() {
          return loadTimeData.getBoolean('isUpstreamVerticalTabsFeatureEnabled')
              ? 'vertical_tabs.enabled'
              : 'brave.tabs.vertical_tabs_enabled'
        },
      },
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
  declare private verticalTabsEnabledForWindow_: boolean
  declare private hasVerticalTabsWindowOverride_: boolean
  declare private verticalTabsHideCompletelyWhenCollapsedPref_:
      PrefObject<boolean>|undefined
  declare private verticalTabsShowToggleButtonPref_: PrefObject<boolean>|undefined
  declare private scrollableHorizontalTabStripPref_: PrefObject<boolean>|undefined
  declare private floatingModePrefKey_: string
  declare private verticalTabsEnabledPrefKey_: string

  override connectedCallback() {
    super.connectedCallback()
    sendWithPromise<boolean>('getIsVerticalTabsToggleEnabled').then(
        (enabled: boolean) => { this.verticalTabsToggleEnabled_ = enabled })
    this.addWebUiListener(
        'vertical-tabs-toggle-enabled-changed',
        (enabled: boolean) => { this.verticalTabsToggleEnabled_ = enabled })
    this.addWebUiListener(
        'vertical-tabs-window-state-changed',
        (state: VerticalTabsWindowState) => {
          this.setVerticalTabsWindowState_(state)
        })
    sendWithPromise<VerticalTabsWindowState>('getVerticalTabsWindowState')
        .then((state: VerticalTabsWindowState) => {
          this.setVerticalTabsWindowState_(state)
        })

    this.mirrorPrefs({
      // Mirror based on which backend is active
      [this.verticalTabsEnabledPrefKey_]: 'verticalTabsEnabledPref_',
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

  private isUpstreamVerticalTabsFeatureEnabled_() {
    return loadTimeData.getBoolean('isUpstreamVerticalTabsFeatureEnabled')
  }

  // The note, the switch sub-label, and the options heading share this.
  // Require the layouts to differ as well as the override, so a pref update
  // that has already caught up hides them before the window-state event
  // arrives.
  private showVerticalTabsWindowStatus_(
      hasOverride: boolean, windowEnabled: boolean,
      prefEnabled: boolean|undefined) {
    return !this.isUpstreamVerticalTabsFeatureEnabled_() && hasOverride &&
        prefEnabled !== undefined &&
        this.verticalTabsLayoutsDiffer_(windowEnabled, prefEnabled)
  }

  // This window's layout does not match the shared switch.
  private verticalTabsLayoutsDiffer_(
      windowEnabled: boolean, prefEnabled: boolean) {
    return windowEnabled !== prefEnabled
  }

  // Shown on the shared switch only while this window uses the other layout.
  private verticalTabsSwitchSublabel_(
      hasOverride: boolean, windowEnabled: boolean,
      prefEnabled: boolean|undefined) {
    if (!this.showVerticalTabsWindowStatus_(
            hasOverride, windowEnabled, prefEnabled)) {
      return ''
    }
    return loadTimeData.getString(
        'appearanceSettingsTabsVerticalTabsSwitchSublabel')
  }

  // The option list still applies to vertical windows when this window's
  // layout does not. Same condition as the note under the switch.
  private showVerticalTabsOptionsHeading_(
      hasOverride: boolean, windowEnabled: boolean,
      prefEnabled: boolean|undefined) {
    return this.showVerticalTabsWindowStatus_(
        hasOverride, windowEnabled, prefEnabled)
  }

  private verticalTabsWindowTitle_(windowEnabled: boolean) {
    return loadTimeData.getString(
        windowEnabled ?
            'appearanceSettingsTabsVerticalTabsThisWindowVerticalTitle' :
            'appearanceSettingsTabsVerticalTabsThisWindowHorizontalTitle')
  }

  private verticalTabsWindowAction_(windowEnabled: boolean) {
    return loadTimeData.getString(
        windowEnabled ?
            'appearanceSettingsTabsVerticalTabsUseHorizontalForWindow' :
            'appearanceSettingsTabsVerticalTabsUseVerticalForWindow')
  }

  private showVerticalTabsOptions_(
      prefEnabled: boolean, windowEnabled: boolean) {
    if (this.isUpstreamVerticalTabsFeatureEnabled_()) {
      return prefEnabled
    }
    return prefEnabled || windowEnabled
  }

  private setVerticalTabsWindowState_(state: VerticalTabsWindowState) {
    this.verticalTabsEnabledForWindow_ = state.enabled
    this.hasVerticalTabsWindowOverride_ = state.hasOverride
  }

  private onFollowVerticalTabsSetting_() {
    chrome.send('clearVerticalTabsWindowOverride')
  }

  // Mirrors upstream's SettingsAppearancePageElement.showEverythingMenuToggle_()
  // (chrome/browser/resources/settings/appearance_page/appearance_page.ts).
  // will show toggle button for everything_menu.pinned_to_tabstrip, and it'll
  // show "Show saved tab groups button" item
  private showEverythingMenuToggle_() {
    return this.isUpstreamVerticalTabsFeatureEnabled_() &&
        !loadTimeData.getBoolean('showOrganizerPanelEnabled') &&
        loadTimeData.getBoolean('showEverythingMenuEnabled')
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
