/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveAccountHandler } from './brave_account.mojom-webui.js'
import type { BraveAccountHandlerInterface } from './brave_account.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  handler: BraveAccountHandlerInterface
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  handler: BraveAccountHandlerInterface

  private constructor() {
    this.handler = BraveAccountHandler.getRemote()
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
