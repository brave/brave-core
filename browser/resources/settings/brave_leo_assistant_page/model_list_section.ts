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
      },
      isOllamaConnected_: {
        type: Boolean,
        value: false
      },
      syncingWithOllama_: {
        type: Boolean,
        value: false
      },
      syncButtonText_: {
        type: String,
        computed: 'computeSyncButtonText_(syncingWithOllama_)'
      },
      ollamaAutoSync_: {
        type: Boolean,
        value: false
      },
      ollamaSyncPref_: {
        type: Object,
        value: function() {
          return {
            key: 'ollama_sync_enabled',
            type: 'boolean',
            value: false
          }
        }
      }
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare customModelsList_: Model[]
  declare isEditingModelIndex_: number | null
  declare showModelConfig_: boolean
  declare isOllamaConnected_: boolean
  declare syncingWithOllama_: boolean
  declare syncButtonText_: string
  declare ollamaAutoSync_: boolean
  declare ollamaSyncPref_: any

  override ready() {
    super.ready()

    const settingsHelper = this.browserProxy_.getSettingsHelper()

    settingsHelper.getCustomModels().then((value: { models: Model[] }) => {
      this.customModelsList_ = value.models
    })

    this.browserProxy_
      .getCallbackRouter()
      .onModelListChanged.addListener((models: Model[]) => {
        this.customModelsList_ = models
      })

    // Check Ollama connection on page load
    this.checkOllamaConnection_()
  }

  async onModelConfigSave_(e: { detail: { modelConfig: Model } }) {
    // Determine the action based on the editing index
    // Need to do explicit null check because 0 is a valid index
    const isEditing = this.isEditingModelIndex_ !== null

    // Since model-config-ui is conditionally rendered, we use this.$$ API to access the element
    const modelConfigElement: any = this.$$('#model-config-ui')

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

  private async checkOllamaConnection_() {
    try {
      const result = await this.browserProxy_.checkOllamaConnection()
      this.isOllamaConnected_ = result.connected
    } catch (error) {
      console.error('Failed to check Ollama connection:', error)
      this.isOllamaConnected_ = false
    }
  }

  private async handleSyncWithOllama_() {
    console.log('Starting Ollama sync...')
    this.syncingWithOllama_ = true

    try {
      const result = await this.browserProxy_.syncOllamaModels()
      console.log('Sync result:', result)

      if (result.success && result.addedCount !== undefined) {
        // Show success message
        console.log(`Successfully added ${result.addedCount} models from Ollama`)
        // Optionally show a toast or notification here
      } else {
        console.error('Failed to sync with Ollama:', result.error)
        // Optionally show error message to user
      }
    } catch (error) {
      console.error('Error syncing with Ollama:', error)
    } finally {
      this.syncingWithOllama_ = false
      console.log('Ollama sync completed')
    }
  }

  private computeSyncButtonText_(syncing: boolean): string {
    return syncing ? 'Syncing...' : 'Sync with Ollama'
  }

  private async handleOllamaToggleChange_(e: any) {
    console.log('Ollama toggle change event:', e)
    console.log('Event detail:', e.detail)
    console.log('Event target:', e.target)
    console.log('Event target checked:', e.target?.checked)

    // Get the new state from the event target's checked property
    const newState = e.target?.checked ?? false
    console.log('Toggle state changed to:', newState)

    // Update our internal state and the fake pref
    this.ollamaAutoSync_ = newState
    this.set('ollamaSyncPref_.value', newState)

    if (newState) {
      console.log('Syncing Ollama models...')
      // Sync models when enabled
      await this.handleSyncWithOllama_()
    } else {
      console.log('Removing Ollama models...')
      // Remove Ollama models when disabled
      await this.removeOllamaModels_()
    }
  }

  private async removeOllamaModels_() {
    // Find all Ollama models and remove them by index
    for (let i = this.customModelsList_.length - 1; i >= 0; i--) {
      const model = this.customModelsList_[i]
      if (model.options.customModelOptions?.endpoint.url === 'http://localhost:11434/v1/chat/completions') {
        await this.browserProxy_.getSettingsHelper().deleteCustomModel(i)
      }
    }
  }

  private isOllamaModel_(model: Model): boolean {
    return !!(model.options.customModelOptions?.endpoint.url === 'http://localhost:11434/v1/chat/completions')
  }
}

customElements.define(ModelListSection.is, ModelListSection)
