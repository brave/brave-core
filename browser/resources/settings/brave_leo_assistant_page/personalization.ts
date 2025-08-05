/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { assert } from 'chrome://resources/js/assert.js';
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from
  'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import '//resources/cr_elements/md_select.css.js'

import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'

import { BaseMixin } from '../base_mixin.js'
import { SettingsToggleButtonElement } from
  '../controls/settings_toggle_button.js'
import { Router } from '../router.js'
import { loadTimeData } from '../i18n_setup.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl,
  PremiumStatus,
  ModelWithSubtitle,
  PremiumInfo,
  ModelAccess,
  Model
} from './brave_leo_assistant_browser_proxy.js'
import './customization_subpage.js'
import { getTemplate } from './personalization.html.js'
import {SettingsViewMixin} from '../settings_page/settings_view_mixin.js'

const BraveLeoPersonalizationBase =
  PrefsMixin(I18nMixin(BaseMixin(SettingsViewMixin(PolymerElement))))

class BraveLeoPersonalization extends BraveLeoPersonalizationBase {
  static get is() {
    return 'brave-leo-personalization'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      selectedModelDisplayName_: {
        type: String,
        computed: 'computeDisplayName_(models_, ' +
          'defaultModelKeyPrefValue_)'
      },
      isPremiumUser_: {
        type: Boolean,
        value: false,
        computed: 'computeIsPremiumUser_(premiumStatus_)'
      },
      defaultModelKeyPrefValue_: String,
      models_: {
        readOnly: true,
        type: Array,
      },
      isHistoryFeatureEnabled_: {
        type: Boolean,
        value: () => loadTimeData.getBoolean(
          'isLeoAssistantHistoryAllowed')
      },
    }
  }

  private declare isPremiumUser_: boolean

  declare isHistoryFeatureEnabled_: boolean
  declare selectedModelDisplayName_: string
  declare defaultModelKeyPrefValue_: string
  declare models_: ModelWithSubtitle[]
  premiumStatus_: PremiumStatus = PremiumStatus.Unknown
  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    this.updateCurrentPremiumStatus()
    this.fetchModelsWithSubtitles_()

    this.browserProxy_.getSettingsHelper().getDefaultModelKey()
      .then((value: { key: string }) => {
        this.defaultModelKeyPrefValue_ = value.key
      })

    this.browserProxy_
      .getCallbackRouter()
      .onDefaultModelChanged.addListener((newKey: string) => {
        this.defaultModelKeyPrefValue_ = newKey
      })

    // To avoid having a seperate event for modelWithSubtitles changing,
    // we can listen to the modelListChanged event.
    this.browserProxy_
      .getCallbackRouter()
      .onModelListChanged.addListener(() => {
        this.fetchModelsWithSubtitles_()
      })

    // Since there is no server-side event for premium status changing,
    // we should check often. And since purchase or login is performed
    // in a separate WebContents, we can check when focus is returned
    // here.
    window.addEventListener('focus', () => {
      this.updateCurrentPremiumStatus()
    })
  }

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'customization':
        return this.shadowRoot!.querySelector('#manageCustomization')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
  }

  private fetchModelsWithSubtitles_() {
    this.browserProxy_.getSettingsHelper().getModelsWithSubtitles()
      .then((value: { models: ModelWithSubtitle[]; }) => {
        this.models_ = value.models
      })
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


  private updateCurrentPremiumStatus() {
    this.browserProxy_.getSettingsHelper().getPremiumStatus()
      .then((value: { status: PremiumStatus; info: PremiumInfo | null; }) => {
        this.premiumStatus_ = value.status
      })
  }

  private isLeoModel_(model: Model) {
    return model.options.leoModelOptions !== undefined
  }

  computeIsPremiumUser_() {
    if (this.premiumStatus_ === PremiumStatus.Active ||
        this.premiumStatus_ === PremiumStatus.ActiveDisconnected) {
      return true
    }

    return false
  }

  shouldShowModelPremiumLabel_(modelAccess: ModelAccess) {
    return (modelAccess === ModelAccess.PREMIUM && !this.isPremiumUser_)
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

  openCustomizationPage_() {
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_LEO_CUSTOMIZATION);
  }
}

customElements.define(BraveLeoPersonalization.is, BraveLeoPersonalization)
