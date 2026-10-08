// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { WebUiListenerMixin } from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import { PrefServiceObserverMixin } from '/shared/settings/prefs2/pref_service_observer_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { getTemplate } from './playlist.html.js'

const BravePlaylistPageBase = WebUiListenerMixin(
  I18nMixin(PrefServiceObserverMixin(PolymerElement))
)

/**
 * 'setting-brave-content-playlist' is the settings page containing settings for Playlist
 */
class SettingsBraveContentPlaylistElement extends BravePlaylistPageBase {
  static get is () {
    return 'settings-brave-content-playlist'
  }

  static get template () {
    return getTemplate()
  }

  static get properties () {
    return {
      playlistEnabledPref_: Object,
    }
  }

  private declare playlistEnabledPref_:
      chrome.settingsPrivate.PrefObject<boolean> | undefined

  override connectedCallback () {
    super.connectedCallback()
    this.mirrorPref('brave.playlist.enabled', 'playlistEnabledPref_')
  }

  private isPlaylistManaged_(
      pref: chrome.settingsPrivate.PrefObject): boolean {
    return pref &&
        pref.enforcement === chrome.settingsPrivate.Enforcement.ENFORCED;
  }
}

customElements.define(SettingsBraveContentPlaylistElement.is, SettingsBraveContentPlaylistElement)
