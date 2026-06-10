// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWebKitMessageWithReply } from '//ios/web/public/js_messaging/resources/utils.js'

const sendMessage = (methodId: string, data: object): Promise<any> => {
  return sendWebKitMessageWithReply('BraveSkusMessageHandler', {
    method_id: methodId,
    data: data,
  })
}

// Setup a window.chrome object if it doesn't exist
if (!(window as any).chrome) {
  ;(window as any).chrome = {}
}

Object.defineProperty((window as any).chrome, 'braveSkus', {
  enumerable: false,
  configurable: false,
  writable: false,
  value: {
    refresh_order(orderId: string): Promise<any> {
      return sendMessage('refreshOrder', { orderId: orderId })
    },

    fetch_order_credentials(orderId: string): Promise<any> {
      return sendMessage('fetchOrderCredentials', { orderId: orderId })
    },

    prepare_credentials_presentation(
      domain: string,
      path: string,
    ): Promise<any> {
      return sendMessage('prepareCredentialsPresentation', {
        domain: domain,
        path: path,
      })
    },

    credential_summary(domain: string): Promise<any> {
      return sendMessage('credentialsSummary', { domain: domain })
    },
  },
})
