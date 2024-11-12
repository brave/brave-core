// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/md_select.css.js'
import 'chrome://resources/brave/leo.bundle.js'
import {assert} from 'chrome://resources/js/assert.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'
import {Router} from '../router.js'
import {loadTimeData} from '../i18n_setup.js'
import {routes} from '../route.js';
import {getTemplate} from './brave_leo_assistant_page.html.js'
import {BraveLeoAssistantBrowserProxy, BraveLeoAssistantBrowserProxyImpl, PremiumStatus, ModelWithSubtitle, PremiumInfo, ModelAccess, Model}
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
        prefs: {
          type: Object,
          notify: true,
        },
        leoAssistantShowOnToolbarPref_: {
          type: Boolean,
          value: false,
          notify: true,
        },
        selectedModelDisplayName_: {
          type: String,
          computed: 'computeDisplayName_(models_, defaultModelKeyPrefValue_)'
        },
        isPremiumUser_: {
          type: Boolean,
          value: false,
          computed: 'computeIsPremiumUser_(premiumStatus_)'
        }
      }
    }

    private isPremiumUser_: boolean

    isHistoryFeatureEnabled_: boolean =
      loadTimeData.getBoolean('isLeoAssistantHistoryAllowed')

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
      this.fetchModelsWithSubtitles_()

      this.addWebUiListener('settings-brave-leo-assistant-changed',
      (isLeoVisible: boolean) => {
        this.leoAssistantShowOnToolbarPref_ = isLeoVisible
      })

      this.browserProxy_.getSettingsHelper().getManageUrl()
        .then((value: { url: string}) => {
          this.manageUrl_ = value.url
        })

      this.browserProxy_.getSettingsHelper().getDefaultModelKey()
        .then((value: { key: string }) => {
          this.defaultModelKeyPrefValue_ = value.key
        })

      this.browserProxy_
        .getCallbackRouter()
        .onDefaultModelChanged.addListener((newKey: string) => {
          this.defaultModelKeyPrefValue_ = newKey
        })

      // To avoid having a seperate event for modelWithSubtitles changing, we
      // can listen to the modelListChanged event.
      this.browserProxy_
        .getCallbackRouter()
        .onModelListChanged.addListener(() => {
          this.fetchModelsWithSubtitles_()
        })

      // Since there is no server-side event for premium status changing,
      // we should check often. And since purchase or login is performed in
      // a separate WebContents, we can check when focus is returned here.
      window.addEventListener('focus', () => {
        this.updateCurrentPremiumStatus()
      })
    }

    private fetchModelsWithSubtitles_() {
      this.browserProxy_.getSettingsHelper().getModelsWithSubtitles()
        .then((value: { models: ModelWithSubtitle[]; }) => {
          this.models_ = value.models
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
      const foundEntry = this.models_?.find(
        (entry) => {
          return entry.model.key === this.defaultModelKeyPrefValue_
        }
      )

      return foundEntry?.model.displayName
    }

    onModelSelectionChange_(e: any) {
      this.browserProxy_.getSettingsHelper().setDefaultModelKey(e.value)
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

    private isLeoModel_(model: Model) {
      return model.options.leoModelOptions !== undefined
    }

    onLeoAssistantShowOnToolbarChange_(e: any) {
      e.stopPropagation()
      this.browserProxy_.toggleLeoIcon()
    }

    openAutocompleteSetting_() {
      Router.getInstance().navigateTo(routes.APPEARANCE, new URLSearchParams("highlight=#autocomplete-suggestion-sources"))
    }

    computeIsPremiumUser_() {
      if (this.premiumStatus_ === PremiumStatus.Active || this.premiumStatus_ === PremiumStatus.ActiveDisconnected) {
        return true
      }

      return false
    }

    shouldShowModelPremiumLabel_(modelAccess: ModelAccess) {
      return (modelAccess === ModelAccess.PREMIUM && !this.isPremiumUser_)
    }

    openManageAccountPage_() {
      window.open(this.manageUrl_, "_self", "noopener noreferrer")
    }

    private onStorageEnabledChange_(event: Event) {
      const target = event.target
      assert(target instanceof SettingsToggleButtonElement);
      // Confirm that the user knows conversation history will be permanently
      // deleted.
      if (!target?.checked) {
        if (!confirm(this.i18n('braveLeoAssistantHistoryPreferenceConfirm'))) {
          target.checked = !target.checked
        }
      }
    }
}

customElements.define(
  BraveLeoAssistantPageElement.is, BraveLeoAssistantPageElement)
