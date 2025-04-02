/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveAccountHandler } from '../brave_account.mojom-webui.js'
import type { BraveAccountHandlerInterface } from '../brave_account.mojom-webui.js'
import { PasswordStrengthMeterHandler } from '../password_strength_meter.mojom-webui.js'
import type { PasswordStrengthMeterHandlerInterface } from '../password_strength_meter.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  account_handler: BraveAccountHandlerInterface
  password_strength_meter_handler: PasswordStrengthMeterHandlerInterface
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  account_handler: BraveAccountHandlerInterface
  password_strength_meter_handler: PasswordStrengthMeterHandlerInterface

  private constructor() {
    this.account_handler = BraveAccountHandler.getRemote()
    this.password_strength_meter_handler = PasswordStrengthMeterHandler.getRemote()
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
