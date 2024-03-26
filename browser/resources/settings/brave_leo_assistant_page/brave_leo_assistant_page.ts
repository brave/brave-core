// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/md_select.css.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';
import {CrSettingsPrefs} from '/shared/settings/prefs/prefs_types.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {getTemplate} from './brave_leo_assistant_page.html.js'
import {BraveLeoAssistantBrowserProxy, BraveLeoAssistantBrowserProxyImpl, PremiumStatus, ModelWithSubtitle, PremiumInfo, ModelAccess}
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
        },
        shouldShowManageSubscriptionLink_: {
          type: Boolean,
          value: false,
          computed: 'computeShouldShowManageSubscriptionLink_(premiumStatus_)'
        }
      }
    }

    leoAssistantShowOnToolbarPref_: boolean
    defaultModelKeyPrefValue_: string
    models_: ModelWithSubtitle[]
    premiumStatus_: PremiumStatus = PremiumStatus.Unknown
    browserProxy_: BraveLeoAssistantBrowserProxy =
      BraveLeoAssistantBrowserProxyImpl.getInstance()
    manageUrl_: string | undefined = undefined

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
      this.updateCurrentPremiumStatus()

      this.addWebUiListener('settings-brave-leo-assistant-changed',
      (isLeoVisible: boolean) => {
        this.leoAssistantShowOnToolbarPref_ = isLeoVisible
      })

      this.browserProxy_.getSettingsHelper().getModelsWithSubtitles()
        .then((value: { models: ModelWithSubtitle[]; }) => {
          this.models_ = value.models
        })

      this.browserProxy_.getSettingsHelper().getManageUrl()
        .then((value: { url: string}) => {
          this.manageUrl_ = value.url
        })

      CrSettingsPrefs.initialized
        .then(() => {
          this.defaultModelKeyPrefValue_ = this.getPref(MODEL_PREF_PATH).value
        })

      // Since there is no server-side event for premium status changing,
      // we should check often. And since purchase or login is performed in
      // a separate WebContents, we can check when focus is returned here.
      window.addEventListener('focus', () => {
        this.updateCurrentPremiumStatus()
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
        (model) => model.model.key === this.defaultModelKeyPrefValue_
      )
      if (!model) {
        return '' // It should appear as if nothing is selected
      }
      return model.model.displayName
    }

    isModelPremium_(modelAccess: ModelAccess) {
      if (modelAccess === ModelAccess.PREMIUM) {
        return true
      }

      return false
    }

    onModelSelectionChange_(e: any) {
      this.setPrefValue(MODEL_PREF_PATH, e.value)
      this.defaultModelKeyPrefValue_ = e.value
    }

    private updateShowLeoAssistantIcon_() {
      this.browserProxy_.getLeoIconVisibility().then((result) => {
        this.leoAssistantShowOnToolbarPref_ = result
      })
    }

    private updateCurrentPremiumStatus() {
      this.browserProxy_.getSettingsHelper().getPremiumStatus().then((value: { status: PremiumStatus; info: PremiumInfo | null; }) => {
        this.premiumStatus_ = value.status
      })
    }

    onLeoAssistantShowOnToolbarChange_(e: any) {
      e.stopPropagation()
      this.browserProxy_.toggleLeoIcon()
    }

    openAutocompleteSetting_() {
      Router.getInstance().navigateTo(routes.APPEARANCE, new URLSearchParams("highlight=#autocomplete-suggestion-sources"))
    }

    computeShouldShowManageSubscriptionLink_() {
      if (this.premiumStatus_ === PremiumStatus.Active) {
        return true
      }

      return false
    }

    openManageAccountPage_() {
      window.open(this.manageUrl_, "_self", "noopener noreferrer")
    }
}

customElements.define(
  BraveLeoAssistantPageElement.is, BraveLeoAssistantPageElement)
