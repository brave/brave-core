// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/md_select.css.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PrefsMixin} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {CrSettingsPrefs} from 'chrome://resources/cr_components/settings_prefs/prefs_types.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {getTemplate} from './brave_leo_assistant_page.html.js'
import {BraveLeoAssistantBrowserProxy, BraveLeoAssistantBrowserProxyImpl, Model}
  from './brave_leo_assistant_browser_proxy.js'
import 'chrome://resources/brave/leo.bundle.js'
import {SettingsRoutes, Router, Route} from '../router.js';
import {routes} from '../route.js';

const MODEL_PREF_PATH = 'brave.ai_chat.default_model_key'

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
        selectedModelDisplayName_: {
          type: String,
          computed: 'computeDisplayName_(models_, defaultModelKeyPrefValue_)'
        }
      }
    }

    leoAssistantShowOnToolbarPref_: boolean
    defaultModelKeyPrefValue_: string
    models_: Model[]

    browserProxy_: BraveLeoAssistantBrowserProxy =
      BraveLeoAssistantBrowserProxyImpl.getInstance()

    onResetAssistantData_() {
      const message =
        this.i18n('braveLeoAssistantResetAndClearDataConfirmationText')
      if(window.confirm(message)) {
        this.browserProxy_.resetLeoData()
      }
    }

    override ready () {
      super.ready()

      this.updateShowLeoAssistantIcon_()

      this.addWebUiListener('settings-brave-leo-assistant-changed',
      (isLeoVisible: boolean) => {
        this.leoAssistantShowOnToolbarPref_ = isLeoVisible
      })

      this.browserProxy_.getModels().then((models) => this.models_ = models)

      CrSettingsPrefs.initialized
        .then(() => {
          this.defaultModelKeyPrefValue_ = this.getPref(MODEL_PREF_PATH).value
        })
    }

    itemPref_(enabled: boolean) {
      return {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: enabled,
      }
    }

    computeDisplayName_() {
      const model = this.models_?.find(
        (m) => m.key === this.defaultModelKeyPrefValue_
      )
      if (!model) {
        return '' // It should appear as if nothing is selected
      }
      return model.display_name
    }

    isModelSelected_(modelKey: string) {
      return (modelKey === this.defaultModelKeyPrefValue_)
    }

    getModelSubtitle_(modelKey: string) {
      return this.i18n(`braveLeoModelSubtitle-${modelKey}`)
    }

    onModelSelectionChange_(e: any) {
      this.setPrefValue(MODEL_PREF_PATH, e.detail.value)
      this.defaultModelKeyPrefValue_ = e.detail.value
    }

    private updateShowLeoAssistantIcon_() {
      this.browserProxy_.getLeoIconVisibility().then((result) => {
        this.leoAssistantShowOnToolbarPref_ = result
      })
    }

    onLeoAssistantShowOnToolbarChange_(e: any) {
      e.stopPropagation()
      this.browserProxy_.toggleLeoIcon()
    }

    openAutocompleteSetting_() {
      Router.getInstance().navigateTo(routes.APPEARANCE, new URLSearchParams("highlight=#autocomplete-suggestion-sources"))
    }
}

customElements.define(
  BraveLeoAssistantPageElement.is, BraveLeoAssistantPageElement)
