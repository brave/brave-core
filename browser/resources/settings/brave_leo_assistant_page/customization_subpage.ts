/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { I18nMixin, I18nMixinInterface } from
  'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from
  'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { PrefsMixin, PrefsMixinInterface } from
  '/shared/settings/prefs/prefs_mixin.js'
import { BaseMixin, BaseMixinInterface } from '../base_mixin.js'
import {
  Customizations,
  CustomizationOperationError,
  MAX_RECORD_LENGTH
} from '../customization_settings.mojom-webui.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl
} from './brave_leo_assistant_browser_proxy.js'
import { getTemplate } from './customization_subpage.html.js'
import './memory_section.js'

const BraveLeoCustomizationSubpageBase =
    PrefsMixin(I18nMixin(BaseMixin(PolymerElement))) as {
      new (): PolymerElement & PrefsMixinInterface & I18nMixinInterface &
        BaseMixinInterface
    }

class BraveLeoCustomizationSubpage extends BraveLeoCustomizationSubpageBase {
  static get is() {
    return 'leo-customization-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      // Local properties for user input
      nameInput_: {
        type: String,
        value: '',
      },
      jobInput_: {
        type: String,
        value: '',
      },
      toneInput_: {
        type: String,
        value: '',
      },
      otherInput_: {
        type: String,
        value: '',
      },
      isSaveDisabled_: {
        type: Boolean,
        computed: 'computeIsSaveDisabled_(nameInput_, jobInput_, toneInput_, ' +
          'otherInput_)',
      },
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()

  // Local input properties
  declare nameInput_: string
  declare jobInput_: string
  declare toneInput_: string
  declare otherInput_: string
  declare isSaveDisabled_: boolean

  override ready() {
    super.ready()
    this.setupCallbacks_()
    this.loadCustomizations_()
  }

  setupCallbacks_() {
    const callbackRouter =
      this.browserProxy_.getCustomizationSettingsCallbackRouter()
    callbackRouter.onCustomizationsChanged.addListener(
      (customizations: Customizations) => {
        this.updateInputValues_(customizations)
      })
  }

  loadCustomizations_() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()

    handler.getCustomizations().then(
      (result: { customizations: Customizations }) => {
        this.updateInputValues_(result.customizations)
      })
  }

  updateInputValues_(customizations: Customizations) {
    this.nameInput_ = customizations.name
    this.jobInput_ = customizations.job
    this.toneInput_ = customizations.tone
    this.otherInput_ = customizations.other
  }

  onSave() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()

    // Reset the value to null if the input is empty string.
    const customizations = {
      name: this.nameInput_,
      job: this.jobInput_,
      tone: this.toneInput_,
      other: this.otherInput_,
    }

    handler.setCustomizations(customizations).then(
      (result: { error: CustomizationOperationError | null }) => {
        if (result.error) {
          this.handleCustomizationError_(result.error)
        }
      })
  }

  handleCustomizationError_(error: CustomizationOperationError) {
    // Handle different error types
    switch (error) {
      // This is unlikely to happen since we have a length limit in frontend
      // for immediate feedback.
      case CustomizationOperationError.kInvalidLength:
        console.error('Customization input exceeds length limit')
        break
      default:
        console.error('Unknown customization error:', error)
        break
    }
  }

  private isTooLong(text: string): boolean {
    return text.trim().length > MAX_RECORD_LENGTH
  }

  private onNameInputChanged_(event: { value: string }) {
    this.nameInput_ = event.value
  }
  private onJobInputChanged_(event: { value: string }) {
    this.jobInput_ = event.value
  }
  private onToneInputChanged_(event: { value: string }) {
    this.toneInput_ = event.value
  }

  private onOtherInputChanged_(event: { value: string }) {
    this.otherInput_ = event.value
  }

  private computeIsSaveDisabled_(
    nameInput: string,
    jobInput: string,
    toneInput: string,
    otherInput: string
  ): boolean {
    return this.isTooLong(nameInput) ||
           this.isTooLong(jobInput) ||
           this.isTooLong(toneInput) ||
           this.isTooLong(otherInput)
  }
}

customElements.define(BraveLeoCustomizationSubpage.is,
  BraveLeoCustomizationSubpage)
