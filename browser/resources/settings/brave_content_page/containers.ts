// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_icon_button/cr_icon_button.js'
import '//resources/mojo/skia/public/mojom/skcolor.mojom-webui.js'

import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
import { assert } from '//resources/js/assert.js'
import {
  CrInputElement
} from 'chrome://resources/cr_elements/cr_input/cr_input.js'
import { CrLitElement } from 'chrome://resources/lit/v3_0/lit.rollup.js'

import { ContainersStrings } from '../brave_generated_resources_webui_strings.js'
import {
  Container,
  ContainerOperationError,
  Icon,
} from '../containers.mojom-webui.js'

import backgroundColors from './background_colors.js'
import { getCss } from './containers.css.js'
import { getHtml } from './containers.html.js'
import type { ColorSelectedEvent } from './containers_background_chip.js'
import { ContainersSettingsHandlerBrowserProxy } from './containers_browser_proxy.js'
import type { IconSelectedEvent } from './containers_icon.js'

const SettingsBraveContentContainersElementBase = I18nMixinLit(CrLitElement)

/**
 * 'settings-brave-content-containers' is the settings page containing settings for Containers
 */
export class SettingsBraveContentContainersElement extends SettingsBraveContentContainersElementBase {
  static get is() {
    return 'settings-brave-content-containers'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      containersList_: {
        type: Array,
      },
      editingContainer_: {
        type: Object,
      },
      deletingContainer_: {
        type: Object,
      },
      isEditDialogNameInvalid_: {
        type: Boolean,
      },
      editDialogError_: {
        type: String,
      },
      deleteDialogError_: {
        type: String,
      },
    }
  }

  private browserProxy = ContainersSettingsHandlerBrowserProxy.getInstance()
  accessor containersList_: Container[] = []
  accessor editingContainer_: Container | undefined
  accessor deletingContainer_: Container | undefined
  accessor isEditDialogNameInvalid_ = false
  accessor editDialogError_: string | undefined
  accessor deleteDialogError_: string | undefined

  override connectedCallback() {
    super.connectedCallback()
    this.browserProxy.handler.getContainers().then(({ containers }) => {
      this.onContainersListUpdated_(containers)
    })
    this.browserProxy.callbackRouter.onContainersChanged.addListener(
      this.onContainersListUpdated_.bind(this),
    )
  }

  onContainersListUpdated_(containers: Container[]) {
    this.containersList_ = containers
  }

  onAddContainerClick_() {
    this.editingContainer_ = {
      id: '',
      name: '',
      icon: Icon.kDefault,
      backgroundColor: backgroundColors[0],
    }
    this.editDialogError_ = undefined
  }

  onEditContainerClick_(e: Event) {
    // eslint-disable-next-line @typescript-eslint/dot-notation
    const id = (e.currentTarget as HTMLElement).dataset['id']
    assert(id)
    this.editingContainer_ = this.containersList_.find((c) => c.id === id)
    this.editDialogError_ = undefined
  }

  onDeleteContainerClick_(e: Event) {
    // eslint-disable-next-line @typescript-eslint/dot-notation
    const id = (e.currentTarget as HTMLElement).dataset['id']
    assert(id)
    this.deletingContainer_ = this.containersList_.find((c) => c.id === id)
    this.deleteDialogError_ = undefined
  }

  onCancelDialog_() {
    this.editingContainer_ = undefined
    this.deletingContainer_ = undefined
  }

  onContainerNameInput_(e: InputEvent) {
    assert(this.editingContainer_)
    const input = e.target as CrInputElement
    this.editingContainer_ = {
      ...this.editingContainer_,
      name: input.value,
    }
    this.isEditDialogNameInvalid_ = input.invalid
  }

  onContainersIconSelected_(event: IconSelectedEvent) {
    assert(this.editingContainer_)
    this.editingContainer_ = {
      ...this.editingContainer_,
      icon: event.detail.icon,
    }
  }

  onContainersBackgroundColorSelected_(event: ColorSelectedEvent) {
    assert(this.editingContainer_)
    this.editingContainer_ = {
      ...this.editingContainer_,
      backgroundColor: event.detail.color,
    }
  }

  async onSaveContainerFromDialog_() {
    assert(this.editingContainer_)
    if (!this.editingContainer_.id) {
      const { error } = await this.browserProxy.handler.addContainer(
        this.editingContainer_,
      )
      if (!error) {
        this.editingContainer_ = undefined
      } else {
        this.editDialogError_ = this.formatError_(error)
      }
    } else {
      const { error } = await this.browserProxy.handler.updateContainer(
        this.editingContainer_,
      )
      if (!error) {
        this.editingContainer_ = undefined
      } else {
        this.editDialogError_ = this.formatError_(error)
      }
    }
  }

  async onDeleteContainerFromDialog_() {
    assert(this.deletingContainer_)
    const { error } = await this.browserProxy.handler.removeContainer(
      this.deletingContainer_.id,
    )
    if (!error) {
      this.deletingContainer_ = undefined
    } else {
      this.deleteDialogError_ = this.formatError_(error)
    }
  }

  getEditDialogTitle_(): string {
    return this.editingContainer_?.id
      ? this.i18n(ContainersStrings.SETTINGS_CONTAINERS_EDIT_CONTAINER_LABEL)
      : this.i18n(ContainersStrings.SETTINGS_CONTAINERS_ADD_CONTAINER_LABEL)
  }

  private formatError_(error: ContainerOperationError): string {
    let errorMessage = ''
    switch (error) {
      case ContainerOperationError.kNotFound:
        // This is the only error we expect in normal operation.
        errorMessage = this.i18n(
          ContainersStrings.SETTINGS_CONTAINERS_ERROR_NOT_FOUND,
        )
        break
      default:
        // These errors are not expected and should be fixed. Just display the
        // error code for debugging.
        errorMessage = ContainerOperationError[error]
        break
    }
    return this.i18n(
      ContainersStrings.SETTINGS_CONTAINERS_ERROR_TEMPLATE,
      errorMessage,
    )
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-containers': SettingsBraveContentContainersElement
  }
}

customElements.define(
  SettingsBraveContentContainersElement.is,
  SettingsBraveContentContainersElement,
)
