// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import 'chrome://resources/cr_elements/cr_input/cr_input.js'
import '../settings_shared.css.js'

import type {CrDialogElement} from 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {getTemplate} from './brave_tor_bridges_dialog.html.js'

import {
  BraveTorBrowserProxy,
  BraveTorBrowserProxyImpl
} from './brave_tor_browser_proxy.js'

export interface RequestBridgesDialog {
  $: {
    dialog: CrDialogElement,
  }
}

const RequestBridgesDialogBase = I18nMixin(PrefsMixin(PolymerElement))

export class RequestBridgesDialog extends RequestBridgesDialogBase {
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

  private status_: string
  private captcha_: string
  private captchaResolve_: string
  private renewDisabled_: boolean
  private submitDisabled_: boolean
  public bridges_: Object[]

  private browserProxy_: BraveTorBrowserProxy =
    BraveTorBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    this.status_ = this.i18n('torRequestBridgeDialogWaiting')
    this.bridges_ = []
    this.requestCaptcha_()
  }

  private submitClicked_() {
    this.enableSubmit_(false)
    this.browserProxy_.resolveBridgesCaptcha(this.captchaResolve_).then(
      (response: { bridges: Object[] }) => {
        this.bridges_ = response.bridges
        this.$.dialog.close()
      },
      () => {
        this.requestCaptcha_()
      })
  }

  private cancelClicked_() {
    this.$.dialog.cancel()
  }

  private requestCaptcha_() {
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

  private enableSubmit_(enabled: boolean) {
    this.renewDisabled_ = !enabled
    this.submitDisabled_ = !enabled
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'request-bridges-dialog': RequestBridgesDialog
  }
}

customElements.define(
  RequestBridgesDialog.is, RequestBridgesDialog)
