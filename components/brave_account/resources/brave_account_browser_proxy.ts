/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Authentication } from './brave_account.mojom-webui.js'
import type { AuthenticationInterface } from './brave_account.mojom-webui.js'
import { PasswordStrengthMeter } from './password_strength_meter.mojom-webui.js'
import type { PasswordStrengthMeterInterface } from './password_strength_meter.mojom-webui.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

export interface BraveAccountBrowserProxy {
  authentication: AuthenticationInterface
  password_strength_meter: PasswordStrengthMeterInterface
  closeDialog: () => void
  getInitiatingServiceName: () => string
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  authentication: AuthenticationInterface
  password_strength_meter: PasswordStrengthMeterInterface

  private constructor() {
    this.authentication = Authentication.getRemote()
    this.password_strength_meter = PasswordStrengthMeter.getRemote()
  }

  closeDialog() {
    chrome.send('dialogClose')
  }

  getInitiatingServiceName() {
    return loadTimeData.getString('initiatingServiceName') || 'accounts'
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
