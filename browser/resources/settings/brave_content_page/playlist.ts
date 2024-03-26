// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { WebUiListenerMixin } from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { getTemplate } from './playlist.html.js'

const BravePlaylistPageBase = WebUiListenerMixin(
  I18nMixin(PrefsMixin(PolymerElement))
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
}

customElements.define(SettingsBraveContentPlaylistElement.is, SettingsBraveContentPlaylistElement)
