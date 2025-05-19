/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PasswordStrengthMeterHandler } from '../password_strength_meter.mojom-webui.js'
import type { PasswordStrengthMeterHandlerInterface } from '../password_strength_meter.mojom-webui.js'

export interface BraveAccountBrowserProxy {
  handler: PasswordStrengthMeterHandlerInterface
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  handler: PasswordStrengthMeterHandlerInterface

  private constructor() {
    this.handler = PasswordStrengthMeterHandler.getRemote()
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
