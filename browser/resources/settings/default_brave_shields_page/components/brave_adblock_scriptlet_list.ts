/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { BaseMixin } from '../../base_mixin.js'

import { getTemplate } from './brave_adblock_scriptlet_list.html.js'

import { loadTimeData } from '../../i18n_setup.js'

import {
  Scriptlet,
  BraveAdblockBrowserProxyImpl,
  ErrorCode
} from '../brave_adblock_browser_proxy.js'

import './brave_adblock_scriptlet_editor.js'

const AdblockScriptletListBase = PrefsMixin(
  I18nMixin(BaseMixin(PolymerElement))
)

class AdblockScriptletList extends AdblockScriptletListBase {
  static get is() {
    return 'adblock-scriptlet-list'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      customScriptletsList_: {
        type: Array
      },
      editingScriptlet_: Scriptlet,
      isEditing_: Boolean
    }
  }

  static get observers() {
    return ['onSctiptelsListChanged_(customScriptletsList_)']
  }

  customScriptletsList_: Scriptlet[]
  editingScriptlet_: Scriptlet | null = null
  isEditing_: boolean = false

  browserProxy_ = BraveAdblockBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing[`adblockScriptletList`] = this.shadowRoot
    }

    this.isEditing_ = false
    this.browserProxy_.getCustomScriptlets().then((scriptlets) => {
      this.customScriptletsList_ = scriptlets
    })
  }

  handleAdd_(_: any) {
    this.editingScriptlet_ = new Scriptlet()
    this.isEditing_ = true
  }

  handleEdit_(e: any) {
    this.editingScriptlet_ = this.customScriptletsList_[e.model.index]
    this.isEditing_ = true
  }

  handleDelete_(e: any) {
    if (!loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      const messageText = this.i18n('adblockCustomScriptletDeleteConfirmation')
      if (!confirm(messageText)) {
        return
      }
    }

    this.browserProxy_
      .removeCustomScriptlet(this.customScriptletsList_[e.model.index].name)
      .then((_: ErrorCode) =>
        this.browserProxy_.getCustomScriptlets().then((scriptlets) => {
          this.customScriptletsList_ = scriptlets
        })
      )
  }

  scriptletEditorClosed_(_: any) {
    this.editingScriptlet_ = null
    this.isEditing_ = false
    this.browserProxy_.getCustomScriptlets().then((scriptlets) => {
      this.customScriptletsList_ = scriptlets
    })
  }

  private onSctiptelsListChanged_(scriptlets: Scriptlet[]) {
    this.fire('list-changed', { value: scriptlets })
  }
}

customElements.define(AdblockScriptletList.is, AdblockScriptletList)
