// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {getTemplate} from './bookmark_bar.html.js'

const SettingsBraveAppearanceBookmarkBarElementBase =
  PrefsMixin(I18nMixin(PolymerElement)) as {
    new (): PolymerElement & I18nMixinInterface & PrefsMixinInterface
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
      }
    }
  }

  bookmarkBarStatePref_: chrome.settingsPrivate.PrefObject

  private bookmarkBarShowOptions_ = [
    {
      value: BookmarkBarState.ALWAYS,
      name: this.i18n('appearanceSettingsBookmarBarAlways')
    },
    {
      value: BookmarkBarState.NONE,
      name: this.i18n('appearanceSettingsBookmarBarNever')
    },
    {
      value: BookmarkBarState.NTP,
      name: this.i18n('appearanceSettingsBookmarBarNTP')
    }
  ]
  private bookmarkBarShowEnabledLabel_: string

  static get observers() {
    return [
      'onPrefsChanged_(prefs.bookmark_bar.show_on_all_tabs.value,' +
      ' prefs.brave.always_show_bookmark_bar_on_ntp.value)'
    ]
  }

  override ready() {
    super.ready()
    window.addEventListener('load', this.onLoad_.bind(this));
  }

  private onLoad_() {
    this.setControlValueFromPrefs()
  }

  private getBookmarkBarStateFromPrefs(): BookmarkBarState {
    if (this.getPref(kShowOnAllTabsPrefName).value)
      return BookmarkBarState.ALWAYS

    if (this.getPref(kAlwaysShowBookmarBarPrefName).value)
      return BookmarkBarState.NTP
    return BookmarkBarState.NONE
  }

  private saveBookmarkBarStateToPrefs(state: BookmarkBarState) {
    if (state === BookmarkBarState.ALWAYS) {
      this.setPrefValue(kShowOnAllTabsPrefName, true)
    } else if (state === BookmarkBarState.NTP) {
      this.setPrefValue(kShowOnAllTabsPrefName, false)
      this.setPrefValue(kAlwaysShowBookmarBarPrefName, true)
    } else {
      this.setPrefValue(kShowOnAllTabsPrefName, false)
      this.setPrefValue(kAlwaysShowBookmarBarPrefName, false)
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
  private onShowOptionChanged_() {
    const state = this.bookmarkBarStatePref_.value
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

    this.saveBookmarkBarStateToPrefs(this.bookmarkBarStatePref_.value)
  }

}

customElements.define(SettingsBraveAppearanceBookmarkBarElement.is,
  SettingsBraveAppearanceBookmarkBarElement)
