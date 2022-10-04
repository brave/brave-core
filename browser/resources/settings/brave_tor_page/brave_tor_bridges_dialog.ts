// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.js';
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { I18nMixin } from 'chrome://resources/js/i18n_mixin.js';
import { PrefsMixin } from '../prefs/prefs_mixin.js';
import '../settings_shared.css.js';
import { BraveTorBrowserProxyImpl } from './brave_tor_browser_proxy.js'
import {getTemplate} from './brave_tor_bridges_dialog.html.js'

const RequestBridgesDialogBase = I18nMixin(PrefsMixin(PolymerElement))

class RequestBridgesDialog extends RequestBridgesDialogBase {
  static get is() {
    return 'request-bridges-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      status_: String,
      captcha_: String,
      captchaResolve_: String,
      renewDisabled_: Boolean,
      submitDisabled_: Boolean,
      bridges_: Array
    }
  }

  browserProxy_ = BraveTorBrowserProxyImpl.getInstance()

  ready() {
    super.ready()

    this.status_ = this.i18n('torRequestBridgeDialogWaiting')
    this.bridges_ = null
    this.requestCaptcha_()
  }

  submitClicked_() {
    this.enableSubmit_(false)
    this.browserProxy_.resolveBridgesCaptcha(this.captchaResolve_).then(
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
    this.captchaResolve_ = ''
    this.status_ = this.i18n('torRequestBridgeDialogWaiting')
    this.captcha_ = ''
    this.enableSubmit_(false)
    this.browserProxy_.requestBridgesCaptcha().then((result) => {
      this.captcha_ = result.captcha
      this.status_ = this.i18n('torRequestBridgeDialogSolve')
      this.enableSubmit_(true)
    }, () => {
      this.status_ = this.i18n('torRequestBridgeDialogError')
      this.renewDisabled_ = false
    })
  }

  enableSubmit_(enabled) {
    this.renewDisabled_ = !enabled
    this.submitDisabled_ = !enabled
  }
}

customElements.define(
  RequestBridgesDialog.is, RequestBridgesDialog)
