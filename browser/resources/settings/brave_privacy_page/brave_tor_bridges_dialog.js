// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import { html, PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { I18nMixin } from 'chrome://resources/js/i18n_mixin.js';
import { PrefsMixin } from '../prefs/prefs_mixin.js';
import '../settings_shared_css.js';
import { BraveTorBrowserProxyImpl } from './brave_tor_browser_proxy.js'

const RequestBridgesDialogBase = I18nMixin(PrefsMixin(PolymerElement))

class RequestBridgesDialog extends RequestBridgesDialogBase {
  static get is() {
    return 'request-bridges-dialog'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      bridges_: Array
    }
  }

  browserProxy_ = BraveTorBrowserProxyImpl.getInstance()

  ready() {
    super.ready()

    this.bridges_ = null
    this.requestCaptcha_()
  }

  submitClicked_() {
    this.browserProxy_.resolveBridgesCaptcha(this.$.captchaResolve.value).then(
      (response) => {
        this.bridges_ = response.bridges
        this.$.dialog.close()
      },
      () => {
        this.requestCaptcha_()
      })
  }

  cancelClicked_() {
    this.$.dialog.cancel()
  }

  requestCaptcha_() {
    this.enableSubmit_(false)
    this.browserProxy_.requestBridgesCaptcha().then((result) => {
      this.$.captchaImage.src = result.captcha
      this.enableSubmit_(true)
    })
  }

  enableSubmit_(enabled) {
    this.$.captchaResolve.disabled = !enabled
    this.$.submit.disabled = !enabled
  }
}

customElements.define(
  RequestBridgesDialog.is, RequestBridgesDialog)
