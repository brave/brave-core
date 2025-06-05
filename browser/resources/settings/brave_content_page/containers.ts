// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement } from 'chrome://resources/lit/v3_0/lit.rollup.js'
import { ContainersSettingsPageBrowserProxy } from './containers_browser_proxy.js'
import { Container } from '../containers.mojom-webui.js'
import { getCss } from './containers.css.js'
import { getHtml } from './containers.html.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
import { assert } from '//resources/js/assert.js'

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
      isRemoving_: {
        type: Boolean,
      },
    }
  }

  private browserProxy = ContainersSettingsPageBrowserProxy.getInstance()
  accessor containersList_: Container[] = []
  accessor editingContainer_: Container | undefined
  accessor deletingContainer_: Container | undefined
  accessor isRemoving_ = false

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
    this.editingContainer_ = { id: '', name: '' }
  }

  onEditContainerClick_(e: Event) {
    const id = (e.currentTarget as HTMLElement).dataset['id']
    assert(id)
    this.editingContainer_ = this.containersList_.find((c) => c.id === id)
  }

  onDeleteContainerClick_(e: Event) {
    const id = (e.currentTarget as HTMLElement).dataset['id']
    assert(id)
    this.deletingContainer_ = this.containersList_.find((c) => c.id === id)
  }

  onCancelDialog_() {
    this.editingContainer_ = undefined
    if (!this.isRemoving_) {
      this.deletingContainer_ = undefined
    }
  }

  onContainerNameInput_(e: InputEvent) {
    assert(this.editingContainer_)
    this.editingContainer_ = {
      ...this.editingContainer_,
      name: (e.target as HTMLInputElement).value,
    }
  }

  onSaveContainerFromDialog_() {
    assert(this.editingContainer_)
    this.browserProxy.handler.addOrUpdateContainer(this.editingContainer_)
    this.editingContainer_ = undefined
  }

  async onDeleteContainerFromDialog_() {
    assert(this.deletingContainer_)
    try {
      this.isRemoving_ = true
      await this.browserProxy.handler.removeContainer(
        this.deletingContainer_.id,
      )
    } finally {
      this.isRemoving_ = false
      this.deletingContainer_ = undefined
    }
  }

  getEditDialogTitle_(): string {
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
