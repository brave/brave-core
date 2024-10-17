/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './model_config_ui.js'

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { BaseMixin } from '../base_mixin.js'

import { getTemplate } from './model_list_section.html.js'
import {
  type BraveLeoAssistantBrowserProxy,
  type Model,
  OperationResult,
  BraveLeoAssistantBrowserProxyImpl
} from './brave_leo_assistant_browser_proxy.js'

const ModelListSectionBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class ModelListSection extends ModelListSectionBase {
  static get is() {
    return 'model-list-section'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      customModelsList_: {
        type: Array
      },
      isEditingModelIndex_: {
        type: Number,
        value: null
      },
      showModelConfig_: {
        type: Boolean,
        value: false
      }
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  customModelsList_: Model[]
  isEditingModelIndex_: number | null
  showModelConfig_: boolean

  override ready() {
    super.ready()

    // Fetch the custom models list
    this.browserProxy_
      .getSettingsHelper()
      .getCustomModels()
      .then((value: { models: Model[] }) => {
        this.customModelsList_ = value.models
      })

    this.browserProxy_
      .getCallbackRouter()
      .onModelListChanged.addListener((models: Model[]) => {
        this.customModelsList_ = models
      })
  }

  async onModelConfigSave_(e: { detail: { modelConfig: Model } }) {
    // Determine the action based on the editing index
    // Need to do explicit null check because 0 is a valid index
    const isEditing = this.isEditingModelIndex_ !== null

    // Since model-config-ui is conditionally rendered, we use this.$$ API to access the element
    const modelConfigElement = this.$$('#model-config-ui') as any

    if (!e.detail.modelConfig.options.customModelOptions) {
      console.error('Custom model options are missing')
      return
    }

    let response = null
    if (isEditing) {
      response = await this.browserProxy_
        .getSettingsHelper()
        .saveCustomModel(
          this
            .isEditingModelIndex_ as number /* We can be confident that this is a number because of the null check */,
          e.detail.modelConfig
        )
    } else {
      response = await this.browserProxy_
        .getSettingsHelper()
        .addCustomModel(e.detail.modelConfig)
    }

    if (response.result === OperationResult.InvalidUrl) {
      modelConfigElement.isUrlInvalid = true
      modelConfigElement.invalidUrlErrorMessage = this.i18n(
        'braveLeoAssistantEndpointError'
      )
      return
    }

    this.showModelConfig_ = false
    this.isEditingModelIndex_ = null
  }

  onModelConfigClose_() {
    this.isEditingModelIndex_ = null
    this.showModelConfig_ = false
  }

  handleDelete_(e: any) {
    const messageText = this.i18n('braveLeoAssistantDeleteModelConfirmation')
    const shouldDeleteModel = confirm(messageText)

    if (!shouldDeleteModel) {
      return
    }

    this.browserProxy_.getSettingsHelper().deleteCustomModel(e.model.index)
  }

  handleEdit_(e: any) {
    this.isEditingModelIndex_ = e.model.index
    this.showModelConfig_ = true
  }

  handleAddNewModel_() {
    this.showModelConfig_ = true
  }

  private getEditingModel_(index: number, customModelsList: Model[]) {
    return customModelsList[index] ?? null
  }

  private hasCustomModels_(customModelsList: Model[]) {
    return customModelsList.length > 0
  }
}

customElements.define(ModelListSection.is, ModelListSection)
