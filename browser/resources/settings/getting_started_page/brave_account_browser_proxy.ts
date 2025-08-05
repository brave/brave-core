/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveAccountSettingsHandler } from '../brave_account_settings_handler.mojom-webui.js'
import type { BraveAccountSettingsHandlerInterface } from '../brave_account_settings_handler.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  handler: BraveAccountSettingsHandlerInterface
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  handler: BraveAccountSettingsHandlerInterface

  private constructor() {
    this.handler = BraveAccountSettingsHandler.getRemote()
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
