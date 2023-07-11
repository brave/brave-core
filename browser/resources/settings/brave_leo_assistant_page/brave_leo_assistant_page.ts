// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PrefsMixin} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {getTemplate} from './brave_leo_assistant_page.html.js'
import {BraveLeoAssistantBrowserProxy, BraveLeoAssistantBrowserProxyImpl}
  from './brave_leo_assistant_browser_proxy.js'


const BraveLeoAssistantPageBase =
  WebUiListenerMixin(I18nMixin(PrefsMixin(PolymerElement)))

/**
 * 'settings-brave-leo-assistant-page' is the settings page containing
 * brave's Leo Assistant features.
 */
class BraveLeoAssistantPageElement extends BraveLeoAssistantPageBase {
    static get is() {
        return 'settings-brave-leo-assistant-page'
    }

    static get template() {
        return getTemplate()
    }

    static get properties() {
      return {
        leoAssistantShowOnToolbarPref_: {
          type: Boolean,
          value: false,
          notify: true,
        },
      }
    }

    leoAssistantShowOnToolbarPref_: boolean
    leoAssistantShowPromptsPref_: boolean
    browserProxy_: BraveLeoAssistantBrowserProxy =
      BraveLeoAssistantBrowserProxyImpl.getInstance()

    onResetAssistanceData_() {
      this.browserProxy_.reset().then((result) => {
        if(result) {
          window.alert(this.i18n('braveLeoAssistantResetConfirmed'))
          this.getShowLeoAssistantIcon_()
        }
      })
    }

    override ready () {
      super.ready()

      this.browserProxy_.initLeoAssistant()

      this.browserProxy_.getShowLeoAssistantIcon().then((result) => {
        this.onShowLeoAssistantIcon_(result)
      })

      this.addWebUiListener('settings-brave-leo-assistant-changed',
      () => { this.getShowLeoAssistantIcon_() })
    }

    itemPref_(enabled: boolean) {
      return {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: enabled,
      }
    }

    private getShowLeoAssistantIcon_() {
      this.browserProxy_.getShowLeoAssistantIcon().then((result) => {
        this.onShowLeoAssistantIcon_(result)
      })
    }

    private onShowLeoAssistantIcon_(isLeoIconVisible: boolean) {
      this.leoAssistantShowOnToolbarPref_ = isLeoIconVisible
    }

    onLeoAssistantShowOnToolbarChange_(e: any) {
      e.stopPropagation()
      this.browserProxy_
      .setShowLeoAssistantIcon(!this.leoAssistantShowOnToolbarPref_)
      .then((result) => {
        if(result) {
          this.onShowLeoAssistantIcon_(!this.leoAssistantShowOnToolbarPref_)
        }
      })
    }
}

customElements.define(
  BraveLeoAssistantPageElement.is, BraveLeoAssistantPageElement)
