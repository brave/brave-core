// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  DomRepeatEvent,
  PolymerElement,
} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { getTemplate } from './containers.html.js'
import { ContainersSettingsPageBrowserProxy } from './containers_browser_proxy.js'
import { Container } from '../containers.mojom-webui.js'
import { assert } from '//resources/js/assert.js'
import { CrDialogElement } from '//resources/cr_elements/cr_dialog/cr_dialog.js'

const SettingsBraveContentContainersElementBase = I18nMixin(PolymerElement)

/**
 * 'settings-brave-content-containers' is the settings page containing settings for Containers
 */
export class SettingsBraveContentContainersElement extends SettingsBraveContentContainersElementBase {
  private browserProxy: ContainersSettingsPageBrowserProxy =
    ContainersSettingsPageBrowserProxy.getInstance()

  static get is() {
    return 'settings-brave-content-containers'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      containersList_: {
        type: Array as unknown as Container[],
      },
      editingContainer_: Object as unknown as Container | null,
      deletingContainer_: Object as unknown as Container | null,
      editDialogTitle_: {
        type: String,
        computed: 'computeEditingDialogTitle_(editingContainer_)',
      },
      isRemoving: {
        type: Boolean,
        value: false,
        observer: 'onIsRemovingChanged_',
      },
    }
  }

  declare containersList_: Container[]
  declare editingContainer_: Container | null
  declare deletingContainer_: Container | null
  declare editDialogTitle_: string
  declare isRemoving: boolean

  override ready() {
    super.ready()
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
    this.editingContainer_ = { id: '', name: '' }
  }

  onEditContainerClick_(e: DomRepeatEvent<Container>) {
    // Copy the container to the editingContainer_ to avoid mutating the
    // original container before saving.
    this.editingContainer_ = { ...e.model.item }
  }

  onDeleteContainerClick_(e: DomRepeatEvent<Container>) {
    assert(!this.deletingContainer_)
    this.deletingContainer_ = e.model.item
  }

  onCancelDialog_() {
    this.editingContainer_ = null
    if (!this.isRemoving) {
      this.deletingContainer_ = null
    }
  }

  onSaveContainerFromDialog_() {
    assert(this.editingContainer_)
    if (!this.editingContainer_.id) {
      this.browserProxy.handler.addContainer(this.editingContainer_)
    } else {
      this.browserProxy.handler.updateContainer(this.editingContainer_)
    }
    this.editingContainer_ = null
  }

  async onDeleteContainerFromDialog_() {
    assert(this.deletingContainer_)

    try {
      this.isRemoving = true
      await this.browserProxy.handler.removeContainer(
        this.deletingContainer_.id,
      )
    } finally {
      this.isRemoving = false
      this.deletingContainer_ = null
    }
  }

  computeEditingDialogTitle_(): string {
    return this.editingContainer_?.id
      ? this.i18n('containersEditContainer')
      : this.i18n('containersAddContainer')
  }

  onIsRemovingChanged_(isRemoving: boolean) {
    // Disable the delete dialog X button when removing a container. We do it
    // manually because the dialog does not offer a way to disable this button
    // via property.
    const dialog = this.shadowRoot?.querySelector('#deleteContainerDialog')
    if (dialog) {
      const closeButton = dialog.shadowRoot?.querySelector('cr-icon-button')
      if (closeButton) {
        if (isRemoving) {
          closeButton.setAttribute('disabled', 'true')
        } else {
          closeButton.removeAttribute('disabled')
        }
      }
    }
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
