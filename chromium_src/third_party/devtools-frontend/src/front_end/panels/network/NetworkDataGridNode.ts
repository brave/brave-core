/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as SDK from '../../core/sdk/sdk.js'

type NetworkRequestNodeType = new (...args: any[]) => {
  request(): SDK.NetworkRequest.NetworkRequest
  existingElement(): Element | null
  updateBackgroundColor(): void
}

export function PatchNetworkRequestNode(
  NetworkRequestNode: NetworkRequestNodeType
): any {
  return class extends NetworkRequestNode {
    override updateBackgroundColor(): void {
      const networkManager = SDK.NetworkManager.NetworkManager.forRequest(
        this.request()
      )
      if (networkManager && networkManager.dispatcher) {
        const info = (networkManager.dispatcher as any).adblockInfoForId(
          this.request().requestId()
        )
        const element = this.existingElement() as HTMLElement
        if (info && element) {
          if ('blocked' in info && info.blocked) {
            element.style.color = 'var(--sys-color-error)'
          } else if ('didMatchException' in info && info.didMatchException) {
            element.style.color = 'var(--ref-palette-green60)'
          }
        }
      }
      super.updateBackgroundColor()
    }
  }
}
