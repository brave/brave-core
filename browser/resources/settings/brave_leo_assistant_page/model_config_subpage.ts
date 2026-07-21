/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { I18nMixin, I18nMixinInterface } from
  'chrome://resources/cr_elements/i18n_mixin.js'
import {
  afterNextRender,
  PolymerElement
} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { PrefsMixin, PrefsMixinInterface } from
  '/shared/settings/prefs/prefs_mixin.js'
import { routes } from '../route.js'
import {
  RouteObserverMixin,
  RouteObserverMixinInterface,
  Router
} from '../router.js'
import type { Route } from '../router.js'
import { SettingsViewMixin } from '../settings_page/settings_view_mixin.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl,
  OperationResult
} from './brave_leo_assistant_browser_proxy.js'
import type { Model } from './brave_leo_assistant_browser_proxy.js'
import { getTemplate } from './model_config_subpage.html.js'
import './model_config_ui.js'

const ModelConfigSubpageBase =
    SettingsViewMixin(PrefsMixin(I18nMixin(RouteObserverMixin(PolymerElement)))) as {
      new (): PolymerElement & PrefsMixinInterface & I18nMixinInterface &
        RouteObserverMixinInterface
    }

class ModelConfigSubpage extends ModelConfigSubpageBase {
  static get is() {
    return 'model-config-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      pageTitle_: {
        type: String,
        value: '',
      },
      editingModel_: {
        type: Object,
        value: null,
      },
      editingModelIndex_: {
        type: Number,
        value: null,
      },
      showForm_: {
        type: Boolean,
        value: false,
      },
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()

  declare pageTitle_: string
  declare editingModel_: Model | null
  declare editingModelIndex_: number | null
  declare showForm_: boolean

  override currentRouteChanged(newRoute: Route) {
    if (newRoute !== routes.BRAVE_LEO_ADD_MODEL) {
      // Tear down the form when leaving so Cancel discards in-progress edits.
      this.resetFormState_()
      return
    }

    // Hide first so restamp recreates a fresh form on each visit. Polymer
    // batches synchronous property updates, so show again after the next
    // render once the previous form has been destroyed.
    this.showForm_ = false

    const indexParam =
      Router.getInstance().getQueryParameters().get('index')
    if (indexParam !== null) {
      const index = Number(indexParam)
      this.editingModelIndex_ = index
      this.pageTitle_ = this.i18n('braveLeoAssistantEditModelLabel')
      this.browserProxy_.getSettingsHelper().getCustomModels()
        .then((value: { models: Model[] }) => {
          this.editingModel_ = value.models[index] ?? null
          this.showFormAfterReset_()
        })
    } else {
      this.editingModelIndex_ = null
      this.editingModel_ = null
      this.pageTitle_ = this.i18n('braveLeoAssistantAddModelLabel')
      this.showFormAfterReset_()
    }
  }

  private resetFormState_() {
    this.showForm_ = false
    this.editingModel_ = null
    this.editingModelIndex_ = null
  }

  private showFormAfterReset_() {
    afterNextRender(this, () => {
      // Only show if we are still on the add/edit route.
      if (Router.getInstance().getCurrentRoute() ===
          routes.BRAVE_LEO_ADD_MODEL) {
        this.showForm_ = true
      }
    })
  }

  async onModelConfigSave_(e: { detail: { modelConfig: Model } }) {
    const isEditing = this.editingModelIndex_ !== null

    const modelConfigElement =
      this.shadowRoot!.querySelector('#model-config-ui') as unknown as {
        isUrlInvalid: boolean
        invalidUrlErrorMessage: string
      }

    if (!e.detail.modelConfig.options.customModelOptions) {
      console.error('Custom model options are missing')
      return
    }

    let response = null
    if (isEditing) {
      response = await this.browserProxy_
        .getSettingsHelper()
        .saveCustomModel(
          this.editingModelIndex_ as number,
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

    this.resetFormState_()
    Router.getInstance().navigateTo(routes.BRAVE_LEO_ASSISTANT)
  }

  onModelConfigClose_() {
    this.resetFormState_()
    Router.getInstance().navigateTo(routes.BRAVE_LEO_ASSISTANT)
  }
}

customElements.define(ModelConfigSubpage.is, ModelConfigSubpage)
