/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as SDK from '../../core/sdk/sdk.js'

import { RequestAdblockView } from './RequestAdblockView.js'

type NetworkItemViewType = new (...args: any[]) => {
  request(): SDK.NetworkRequest.NetworkRequest
  appendTab(
    id: string,
    tabTitle: string,
    view: any,
    tabTooltip?: string,
    userGesture?: boolean,
    isCloseable?: boolean,
    isPreviewFeature?: boolean,
    index?: number,
    jslogContext?: string
  ): void
}

export function PatchNetworkItemView(
  NetworkItemView: NetworkItemViewType
): any {
  return class extends NetworkItemView {
    constructor(...args: any[]) {
      super(...args)
      this.appendTab(
        'blocking',
        'Adblock',
        new RequestAdblockView(this.request()),
        'Adblock'
      )
    }
  }
}
