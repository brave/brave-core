/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { BaseMixin } from '../base_mixin.js'
import { routes } from '../route.js'
import { Router } from '../router.js'
import { SettingsViewMixin } from '../settings_page/settings_view_mixin.js'

import { getTemplate } from './model_list_section.html.js'
import {
  BraveLeoAssistantBrowserProxyImpl,
  OLLAMA_ENDPOINT
} from './brave_leo_assistant_browser_proxy.js'
import type {
  BraveLeoAssistantBrowserProxy,
  Model
} from './brave_leo_assistant_browser_proxy.js'

const ModelListSectionBase =
  PrefsMixin(I18nMixin(BaseMixin(SettingsViewMixin(PolymerElement))))

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
      isOllamaConnected_: {
        type: Boolean,
        value: false
      }
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare customModelsList_: Model[]
  declare isOllamaConnected_: boolean

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

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'add-model':
        return this.shadowRoot!.querySelector('#addNewModel')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
  }

  handleDelete_(e: Event & {model: {index: number}}) {
    const messageText = this.i18n('braveLeoAssistantDeleteModelConfirmation')
    const shouldDeleteModel = confirm(messageText)

    if (!shouldDeleteModel) {
      return
    }

    this.browserProxy_.getSettingsHelper().deleteCustomModel(e.model.index)
  }

  handleEdit_(e: Event & {model: {index: number}}) {
    Router.getInstance().navigateTo(
      routes.BRAVE_LEO_ADD_MODEL,
      new URLSearchParams({index: String(e.model.index)})
    )
  }

  handleAddNewModel_() {
    Router.getInstance().navigateTo(routes.BRAVE_LEO_ADD_MODEL)
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

  private isOllamaManagedModel_(
      model: Model, ollamaSyncEnabled: boolean,
      isOllamaConnected: boolean): boolean {
    // Only consider it managed if all three:
    // 1. It points to Ollama endpoint
    // 2. Ollama sync preference is enabled
    // 3. Ollama is actually connected
    const isOllamaEndpoint =
        model.options.customModelOptions?.endpoint === OLLAMA_ENDPOINT
    return isOllamaEndpoint && ollamaSyncEnabled && isOllamaConnected
  }
}

customElements.define(ModelListSection.is, ModelListSection)
