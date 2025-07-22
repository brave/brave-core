/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_dialog.js'
import { BraveAccountErrorDialogElement } from './brave_account_error_dialog.js'
import { RegisterFailureReason } from './brave_account.mojom-webui.js'

export function getHtml(this: BraveAccountErrorDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      alert-message=${(() => {
        if (this.failureReason.kind === 'register') {
           switch (this.failureReason.reason) {
               case RegisterFailureReason.kInitializeBadRequest:
                 return '$i18n{braveAccountErrorDialogInitializeBadRequest}';
               case RegisterFailureReason.kInitializeUnauthorized:
                 return '$i18n{braveAccountErrorDialogInitializeUnauthorized}';
               case RegisterFailureReason.kInitializeForbidden:
                 return '$i18n{braveAccountErrorDialogInitializeForbidden}';
               case RegisterFailureReason.kInitializeInternalServerError:
                 return '$i18n{braveAccountErrorDialogInitializeInternalServerError}';
               case RegisterFailureReason.kInitializeUnknown:
                 return '$i18n{braveAccountErrorDialogInitializeUnknown}';
               case RegisterFailureReason.kInitializeUnexpected:
                 return '$i18n{braveAccountErrorDialogInitializeUnexpected}';
               case RegisterFailureReason.kFinalizeBadRequest:
                 return '$i18n{braveAccountErrorDialogFinalizeBadRequest}';
               case RegisterFailureReason.kFinalizeUnauthorized:
                 return '$i18n{braveAccountErrorDialogFinalizeUnauthorized}';
               case RegisterFailureReason.kFinalizeForbidden:
                 return '$i18n{braveAccountErrorDialogFinalizeForbidden}';
               case RegisterFailureReason.kFinalizeNotFound:
                 return '$i18n{braveAccountErrorDialogFinalizeNotFound}';
               case RegisterFailureReason.kFinalizeInternalServerError:
                 return '$i18n{braveAccountErrorDialogFinalizeInternalServerError}';
               case RegisterFailureReason.kFinalizeUnknown:
                 return '$i18n{braveAccountErrorDialogFinalizeUnknown}';
               case RegisterFailureReason.kFinalizeUnexpected:
                 return '$i18n{braveAccountErrorDialogFinalizeUnexpected}';
               default:
                 return ''
           }
        }
        return ''
      })()}
      dialog-description="$i18n{braveAccountErrorDialogDescription}"
      dialog-title="$i18n{braveAccountErrorDialogTitle}"
    >
      <leo-button slot="buttons" @click=${() => this.fire('back-button-clicked')}>
        $i18n{braveAccountBackButtonLabel}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
