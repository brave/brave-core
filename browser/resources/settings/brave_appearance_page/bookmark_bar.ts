// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PrefServiceObserverMixin, PrefServiceObserverMixinInterface} from '/shared/settings/prefs2/pref_service_observer_mixin.js';
import {PrefService} from '/shared/settings/prefs2/pref_service.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {loadTimeData} from '../i18n_setup.js';

import {getTemplate} from './bookmark_bar.html.js'

const SettingsBraveAppearanceBookmarkBarElementBase =
  PrefServiceObserverMixin(I18nMixin(PolymerElement)) as {
    new (): PolymerElement & I18nMixinInterface & PrefServiceObserverMixinInterface
  }

enum BookmarkBarState {
  ALWAYS = 0,
  NONE = 1,
  NTP = 2,
}

const kAlwaysShowBookmarBarPrefName = 'brave.always_show_bookmark_bar_on_ntp'
const kShowOnAllTabsPrefName = 'bookmark_bar.show_on_all_tabs'

/**
 * 'settings-brave-appearance-bookmark-bar' is the settings page area containing
 * brave's bookmark bar visibility settings in appearance settings.
 */
export class SettingsBraveAppearanceBookmarkBarElement
    extends SettingsBraveAppearanceBookmarkBarElementBase {
  static get is() {
    return 'settings-brave-appearance-bookmark-bar'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /** @private {chrome.settingsPrivate.PrefType} */
      bookmarkBarStatePref_: {
        key: '',
        type: Object,
        value() {
          return {
            key: '',
            type: chrome.settingsPrivate.PrefType.NUMBER,
            value: BookmarkBarState.NTP
          }
        }
      },

      bookmarkBarShowOptions_: {
        readyOnly: true,
        type: Array,
        value() {
          return [
            {
              value: BookmarkBarState.ALWAYS,
              name: loadTimeData.getString('appearanceSettingsBookmarBarAlways')
            },
            {
              value: BookmarkBarState.NONE,
              name: loadTimeData.getString('appearanceSettingsBookmarBarNever')
            },
            {
              value: BookmarkBarState.NTP,
              name: loadTimeData.getString('appearanceSettingsBookmarBarNTP')
            }
          ];
        }
      },

      // Mirrored from the global PrefService. `bookmarkBarStatePref_` above
      // is a synthetic pref derived from these two real ones.
      showOnAllTabsPref_: Object,
      alwaysShowOnNtpPref_: Object,
    }
  }

  declare bookmarkBarStatePref_: chrome.settingsPrivate.PrefObject

  declare private bookmarkBarShowOptions_ :
      Array<{value: BookmarkBarState, name: string}>
  private bookmarkBarShowEnabledLabel_: string
  declare private showOnAllTabsPref_:
      chrome.settingsPrivate.PrefObject<boolean>|undefined
  declare private alwaysShowOnNtpPref_:
      chrome.settingsPrivate.PrefObject<boolean>|undefined

  static get observers() {
    return [
      'onPrefsChanged_(showOnAllTabsPref_.value, alwaysShowOnNtpPref_.value)'
    ]
  }

  override connectedCallback() {
    super.connectedCallback()
    this.mirrorPrefs({
      [kShowOnAllTabsPrefName]: 'showOnAllTabsPref_',
      [kAlwaysShowBookmarBarPrefName]: 'alwaysShowOnNtpPref_',
    })
  }

  private getBookmarkBarStateFromPrefs(): BookmarkBarState {
    if (this.showOnAllTabsPref_?.value)
      return BookmarkBarState.ALWAYS

    if (this.alwaysShowOnNtpPref_?.value)
      return BookmarkBarState.NTP
    return BookmarkBarState.NONE
  }

  private saveBookmarkBarStateToPrefs(state: BookmarkBarState) {
    if (state === BookmarkBarState.ALWAYS) {
      PrefService.getInstance().setPrefValue(kShowOnAllTabsPrefName, true)
    } else if (state === BookmarkBarState.NTP) {
      PrefService.getInstance().setPrefValue(kShowOnAllTabsPrefName, false)
      PrefService.getInstance().setPrefValue(kAlwaysShowBookmarBarPrefName, true)
    } else {
      PrefService.getInstance().setPrefValue(kShowOnAllTabsPrefName, false)
      PrefService.getInstance().setPrefValue(kAlwaysShowBookmarBarPrefName, false)
    }
  }
  private setControlValueFromPrefs() {
    const state = this.getBookmarkBarStateFromPrefs()
    if (this.bookmarkBarStatePref_.value === state)
      return
    this.bookmarkBarStatePref_ = {
      key: '',
      type: chrome.settingsPrivate.PrefType.NUMBER,
      value: state
    };
  }
  private onPrefsChanged_() {
    this.setControlValueFromPrefs()
  }
  private onShowOptionChanged_(e: Event) {
    // Since cr151 `settings-dropdown-menu` is a Lit control that no longer
    // writes back to the bound `pref` object (it persists via `pref-key`). This
    // control uses a synthetic pref, so read the selected value directly from
    // the dropdown instead of the (now stale) `bookmarkBarStatePref_`.
    const state = Number(
      (e.target as unknown as {getSelectedValue(): string})
        .getSelectedValue()) as BookmarkBarState
    if (state === BookmarkBarState.ALWAYS) {
      this.bookmarkBarShowEnabledLabel_ =
        this.i18n('appearanceSettingsBookmarBarAlwaysDesc')
    } else if (state === BookmarkBarState.NTP) {
      this.bookmarkBarShowEnabledLabel_ =
        this.i18n('appearanceSettingsBookmarBarNTPDesc')
    } else {
      this.bookmarkBarShowEnabledLabel_ =
        this.i18n('appearanceSettingsBookmarBarNeverDesc')
    }

    this.saveBookmarkBarStateToPrefs(state)
  }

}

customElements.define(SettingsBraveAppearanceBookmarkBarElement.is,
  SettingsBraveAppearanceBookmarkBarElement)
