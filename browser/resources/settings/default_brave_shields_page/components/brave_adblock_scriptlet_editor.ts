// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import { CrDialogElement } from 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import 'chrome://resources/cr_elements/cr_input/cr_input.js'

import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { getTemplate } from './brave_adblock_scriptlet_editor.html.js'

import {
  Scriptlet,
  BraveAdblockBrowserProxyImpl,
  ErrorCode
} from '../brave_adblock_browser_proxy.js'

interface AdblockScriptletEditor {
  $: {
    dialog: CrDialogElement
  }
}

const AdblockScriptletEditorBase = I18nMixin(PrefsMixin(PolymerElement))

class AdblockScriptletEditor extends AdblockScriptletEditorBase {
  static get is() {
    return 'adblock-scriptlet-editor'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      scriptlet: Scriptlet,
      dialogTitle_: String,
      isScriptletValid_: Boolean,
      scriptletErrorMessage_: String
    }
  }

  scriptlet: Scriptlet
  dialogTitle_: string
  isScriptletValid_: boolean
  scriptletErrorMessage_: string

  scriptletName_: string
  browserProxy_ = BraveAdblockBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()
    this.scriptletName_ = this.scriptlet.name

    if (this.scriptletName_) {
      this.dialogTitle_ = this.i18n('adblockEditCustomScriptletDialogTitle')
    } else {
      this.dialogTitle_ = this.i18n('adblockAddCustomScriptletDialogTitle')
    }

    this.updateError(ErrorCode.kOK)
  }

  updateError(error_code: ErrorCode) {
    this.isScriptletValid_ = error_code === ErrorCode.kOK
    switch (error_code) {
      case ErrorCode.kOK:
        this.scriptletErrorMessage_ = ''
        break
      case ErrorCode.kAlreadyExists:
        this.scriptletErrorMessage_ = this.i18n(
          'adblockEditCustomScriptletAlreadyExistsError'
        )
        break
      case ErrorCode.kInvalidName:
        this.scriptletErrorMessage_ = this.i18n(
          'adblockEditCustomScriptletInvalidNameError'
        )
        break
      case ErrorCode.kNotFound:
        this.scriptletErrorMessage_ = this.i18n(
          'adblockEditCustomScriptletNotFoundError'
        )
        break
    }
  }

  cancelClicked_() {
    this.$.dialog.cancel()
  }

  saveClicked_() {
    if (this.scriptletName_) {
      this.browserProxy_
        .updateCustomScriptlet(this.scriptletName_, this.scriptlet)
        .then((e) => {
          this.updateError(e)
          if (this.isScriptletValid_) {
            this.$.dialog.close()
          }
        })
    } else {
      this.browserProxy_.addCustomScriptlet(this.scriptlet).then((e) => {
        this.updateError(e)
        if (this.isScriptletValid_) {
          this.$.dialog.close()
        }
      })
    }
  }

  validateName_() {
    if (!/^[a-z-_.]*$/.test(this.scriptlet.name)) {
      this.updateError(ErrorCode.kInvalidName)
    } else {
      this.updateError(ErrorCode.kOK)
    }
  }
}

customElements.define(AdblockScriptletEditor.is, AdblockScriptletEditor)
