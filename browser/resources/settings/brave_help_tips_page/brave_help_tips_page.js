/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
 import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
 import {WebUIListenerMixin} from 'chrome://resources/js/web_ui_listener_mixin.js';
 import {PrefsMixin} from '../prefs/prefs_mixin.js';
 
 const SettingBraveHelpTipsPageElementBase = WebUIListenerMixin(PrefsMixin(PolymerElement))


class SettingsBraveHelpTipsPageElement extends SettingBraveHelpTipsPageElementBase {
  static get is() {
    return 'settings-brave-help-tips-page'
  }

  static get template() {
    return html`{__html_template__}`
  }
}

customElements.define(
  SettingsBraveHelpTipsPageElement.is, SettingsBraveHelpTipsPageElement)
