/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { sendWithPromise } from 'chrome://resources/js/cr.js'
import type { CrInputElement } from 'chrome://resources/cr_elements/cr_input/cr_input.js'
import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import * as mojom from './brave_leo_assistant_browser_proxy.js'
import { BaseMixin } from '../base_mixin.js'
import { getTemplate } from './model_config_ui.html.js'

const ModelConfigUIBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

export class ModelConfigUI extends ModelConfigUIBase {
  static get is() {
    return 'model-config-ui'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      label: {
        type: String
      },
      modelRequestName: {
        type: String
      },
      contextSize: {
        type: Number
      },
      modelSystemPrompt: {
        type: String,
        value: ''
      },
      promptTokensEstimate: {
        type: Number,
        value: 0
      },
      promptTokensEstimateString: {
        type: String,
        value: ''
      },
      endpointUrl: {
        type: String
      },
      apiKey: {
        type: String,
        value: ''
      },
      isUrlInvalid: {
        type: Boolean,
        value: false,
      },
      shouldShowUnsafeEndpointModal: {
        type: Boolean,
        value: false,
      },
      shouldShowUnsafeEndpointLabel: {
        type: Boolean,
        value: false,
      },
      invalidUrlErrorMessage: {
        type: String,
        value: ''
      },
      modelItem: {
        type: Object,
        value: null,
        notify: true,
        observer: 'onModelItemChange_'
      },
      isEditing_: {
        type: Boolean,
        value: false,
        readOnly: true,
        computed: 'computeIsEditing_(modelItem)'
      },
      buttonLabel_: {
        type: String,
        computed: 'computeButtonLabel_(isEditing_)'
      }
    }
  }

  label: string
  modelRequestName: string
  contextSize: number
  modelSystemPrompt: string | null
  promptTokensEstimate: number
  promptTokensEstimateString: string
  endpointUrl: string
  apiKey: string
  modelItem: mojom.Model | null
  isEditing_: boolean
  isUrlInvalid: boolean
  shouldShowUnsafeEndpointLabel: boolean
  isValidAsPrivateEndpoint: boolean
  shouldShowUnsafeEndpointModal: boolean
  invalidUrlErrorMessage: string

  override ready() {
    super.ready()
    // If a user previously had --brave-ai-chat-allow-private-ips enabled, but
    // now has it disabled, they should be notified of invalid endpoints
    // immediately upon opening the config view of an impacted model. We should
    // not wait for the user to make a change to the endpoint before informing
    // them that the endpoint is no longer valid.
    this.checkEndpointValidity_()
  }

  async handleClick_() {
    // If the user is attempting to use a private endpoint, we should show a
    // modal warning instructing them to enable the optional feature in order
    // to proceed
    this.shouldShowUnsafeEndpointModal =
      this.isUrlInvalid && this.isValidAsPrivateEndpoint

    if (!this.saveEnabled_()) {
      return
    }

    const mojomUrl = { url: '' }
    mojomUrl.url = this.endpointUrl

    // Send empty string if adding new model
    const modelKey = this.isEditing_ ? this.modelItem?.key : ''

    if (modelKey === undefined) {
      console.error('Model key is undefined')
      return
    }

    const modelConfig: mojom.Model = {
      options: {
        customModelOptions: {
          modelRequestName: this.modelRequestName,
          contextSize: this.contextSize,
          // Determined at runtime based on contextSize
          maxAssociatedContentLength: -1,
          // Determined at runtime based on contextSize
          longConversationWarningCharacterLimit: -1,
          modelSystemPrompt: this.modelSystemPrompt,
          endpoint: mojomUrl,
          apiKey: this.apiKey
        }
      },
      key: modelKey,
      displayName: this.label
    }

    this.fire('save', { modelConfig })
  }

  handleCloseClick_() {
    this.fire('close')
  }

  onModelLabelChange_(e: any) {
    this.label = e.target.value
  }

  onModelRequestNameChange_(e: any) {
    this.modelRequestName = e.target.value
  }

  onModelSystemPromptChange_(e: any) {
    this.modelSystemPrompt = e.target.value
  }

  onContextSizeChange_(e: any) {
    this.contextSize = parseInt(e.target.value, 10);
  }

  onModelServerEndpointChange_(e: any) {
    this.endpointUrl = e.target.value
    this.checkEndpointValidity_()
  }

  onModelApiKeyChange_(e: any) {
    this.apiKey = e.target.value
  }

  constructTokenEstimateString_( e?: any ) {
    let charsLength = 0;
    let tokensLength = 0;

    if ( e?.target?.value ) {
      charsLength = e.target.value.length;
    } else if ( this.modelSystemPrompt ) {
      charsLength = this.modelSystemPrompt.length;
    }

    tokensLength = charsLength > 0 ? Math.ceil(charsLength/4) : 0;

    this.promptTokensEstimate = tokensLength
    this.promptTokensEstimateString = this.i18n(
      'braveLeoAssistantTokensCount',
      this.promptTokensEstimate
    )
  }

  private saveEnabled_() {
    // Make sure all required fields are filled
    return this.label && this.modelRequestName && this.endpointUrl && !this.isUrlInvalid
  }

  private checkEndpointValidity_() {
    const url = this.endpointUrl.trim()
    if (url !== '') {
      sendWithPromise('validateModelEndpoint', { url })
        .then((response: any) => {
          this.isUrlInvalid = !response.isValid
          this.isValidAsPrivateEndpoint = response.isValidAsPrivateEndpoint
          this.shouldShowUnsafeEndpointLabel =
            response.isValidDueToPrivateIPsFeature
          this.invalidUrlErrorMessage =
            this.i18n('braveLeoAssistantEndpointError')
        })
    }
  }

  private onModelItemChange_(newValue: mojom.Model | null) {
    if (newValue?.options.customModelOptions) {
      this.label = newValue.displayName
      this.modelRequestName =
        newValue.options.customModelOptions.modelRequestName
      this.contextSize =
        newValue.options.customModelOptions.contextSize
      this.endpointUrl = newValue.options.customModelOptions.endpoint.url
      this.apiKey = newValue.options.customModelOptions.apiKey
      this.modelSystemPrompt =
        newValue.options.customModelOptions.modelSystemPrompt
    }
    this.constructTokenEstimateString_()
  }

  private computeIsEditing_(modelItem: mojom.Model | null) {
    return !!modelItem
  }

  private computeButtonLabel_(isEditing: boolean) {
    return isEditing
      ? this.i18n('braveLeoAssistantSaveModelButtonLabel')
      : this.i18n('braveLeoAssistantAddModelButtonLabel')
  }
}

customElements.define(ModelConfigUI.is, ModelConfigUI)
