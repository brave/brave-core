/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  PageHandlerFactory,
  PageHandlerRemote,
} from './brave_account.mojom-webui.js'
import type { PageHandlerInterface } from './brave_account.mojom-webui.js'
import { PasswordStrengthMeter } from './password_strength_meter.mojom-webui.js'
import type { PasswordStrengthMeterInterface } from './password_strength_meter.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  handler: PageHandlerInterface
  password_strength_meter: PasswordStrengthMeterInterface
  closeDialog: () => void
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  handler: PageHandlerInterface
  password_strength_meter: PasswordStrengthMeterInterface

  private constructor() {
    this.handler = new PageHandlerRemote()
    PageHandlerFactory.getRemote().createPageHandler(
      (this.handler as PageHandlerRemote).$.bindNewPipeAndPassReceiver(),
    )

    this.password_strength_meter = PasswordStrengthMeter.getRemote()
  }

  closeDialog() {
    chrome.send('dialogClose')
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
