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

import { loadTimeData } from '../../i18n_setup.js'

import {
  Scriptlet,
  BraveAdblockBrowserProxyImpl,
  DEFAULT_SCRIPTLET_MIME,
  ErrorCode
} from '../brave_adblock_browser_proxy.js'

interface AdblockScriptletEditor {
  $: {
    dialog: CrDialogElement
  }
}

const AdblockScriptletEditorBase = I18nMixin(PrefsMixin(PolymerElement))

const MIME_OPTIONS = [
  DEFAULT_SCRIPTLET_MIME,
  'text/css',
  'text/html',
  'application/json',
  'text/plain',
  'application/octet-stream',
  'text/xml',
  'audio/mp3',
  'video/mp4',
  'image/gif',
  'image/png',
].map((value) => ({ value }))

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
      scriptletErrorMessage_: String,
      scriptletName_: String,
      scriptletMime_: String,
      mimeOptions_: {
        type: Array,
        value() {
          return MIME_OPTIONS
        }
      },
    }
  }

  declare scriptlet: Scriptlet
  declare dialogTitle_: string
  declare isScriptletValid_: boolean
  declare scriptletErrorMessage_: string
  declare scriptletName_: string
  declare scriptletMime_: string
  declare mimeOptions_: typeof MIME_OPTIONS

  originalScriptlet_: Scriptlet
  browserProxy_ = BraveAdblockBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {};
      window.testing[`adblockScriptletEditor`] = this.shadowRoot;
    }

    this.originalScriptlet_ = {
      ...this.scriptlet,
      kind: {
        mime: this.scriptlet.kind?.mime || DEFAULT_SCRIPTLET_MIME
      },
    }
    this.scriptletMime_ = this.originalScriptlet_.kind.mime

    if (this.originalScriptlet_.name) {
      this.dialogTitle_ = this.i18n('adblockEditCustomScriptletDialogTitle')
      // Strip the 'user-' prefix and '.js' suffix for display
      let name = this.originalScriptlet_.name
      if (name.toLowerCase().startsWith('user-')) {
        name = name.slice(5)
      }
      if (name.endsWith('.js')) {
        name = name.slice(0, -3)
      }
      this.scriptletName_ = name
    } else {
      this.dialogTitle_ = this.i18n('adblockAddCustomScriptletDialogTitle')
      this.scriptletName_ = ''
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
          'adblockCustomScriptletAlreadyExistsError'
        )
        break
      case ErrorCode.kInvalidName:
        this.scriptletErrorMessage_ = this.i18n(
          'adblockCustomScriptletInvalidNameError'
        )
        break
      case ErrorCode.kNotFound:
        this.scriptletErrorMessage_ = this.i18n(
          'adblockCustomScriptletNotFoundError'
        )
        break
    }
  }

  cancelClicked_() {
    Object.assign(this.scriptlet, this.originalScriptlet_)
    this.$.dialog.cancel()
  }

  saveClicked_() {
    this.updateScriptletBeforeSave_()
    if (!this.isScriptletValid_) {
      return
    }

    if (this.originalScriptlet_.name) {
      this.browserProxy_
        .updateCustomScriptlet(this.originalScriptlet_.name, this.scriptlet)
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
    if (!/^[a-zA-Z0-9-_.]+$/.test(this.scriptletName_)) {
      this.updateError(ErrorCode.kInvalidName)
    } else {
      this.updateError(ErrorCode.kOK)
    }
  }

  mimeEqual_(a: string, b: string) {
    return a === b
  }

  onMimeChange_(e: Event) {
    this.scriptletMime_ = (e.target as HTMLSelectElement).value
  }

  updateScriptletBeforeSave_() {
    this.scriptlet.name = 'user-' + this.scriptletName_ + '.js'
    this.scriptlet.kind = {
      mime: this.scriptletMime_ || DEFAULT_SCRIPTLET_MIME
    }
    this.validateName_()
  }
}

customElements.define(AdblockScriptletEditor.is, AdblockScriptletEditor)
