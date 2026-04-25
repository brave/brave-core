/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Authentication,
  AuthenticationObserverCallbackRouter,
  DialogController,
  Service,
} from './brave_account.mojom-webui.js'
import type {
  AuthenticationInterface,
  DialogControllerInterface,
} from './brave_account.mojom-webui.js'
import { PasswordStrengthMeter } from './password_strength_meter.mojom-webui.js'
import type { PasswordStrengthMeterInterface } from './password_strength_meter.mojom-webui.js'
import { loadTimeData } from '//resources/js/load_time_data.js'

export interface BraveAccountBrowserProxy {
  authentication: AuthenticationInterface
  authenticationObserverCallbackRouter: AuthenticationObserverCallbackRouter
  dialog_controller: DialogControllerInterface
  password_strength_meter: PasswordStrengthMeterInterface
  closeDialog: () => void
  getInitiatingService: () => Service | null
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  authentication: AuthenticationInterface
  authenticationObserverCallbackRouter: AuthenticationObserverCallbackRouter
  dialog_controller: DialogControllerInterface
  password_strength_meter: PasswordStrengthMeterInterface

  private constructor() {
    this.authentication = Authentication.getRemote()
    this.authenticationObserverCallbackRouter =
      new AuthenticationObserverCallbackRouter()
    this.dialog_controller = DialogController.getRemote()
    this.password_strength_meter = PasswordStrengthMeter.getRemote()

    this.authentication.addObserver(
      this.authenticationObserverCallbackRouter.$.bindNewPipeAndPassRemote(),
    )
  }

  closeDialog() {
    this.dialog_controller.closeDialog()
  }

  getInitiatingService(): Service | null {
    const id = 'initiatingService'
    return loadTimeData.valueExists(id)
      ? (loadTimeData.getInteger(id) as Service)
      : null
  }

  static getInstance(): BraveAccountBrowserProxy {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
