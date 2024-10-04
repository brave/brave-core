/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { sendWithPromise } from '//resources/js/cr.js'

export interface BraveAccountBrowserProxy {
  getPasswordStrength(password: string): Promise<number>
}

export class BraveAccountBrowserProxyImpl implements BraveAccountBrowserProxy {
  getPasswordStrength (password: string) {
    return sendWithPromise('getPasswordStrength', password)
  }

  static getInstance() {
    return instance || (instance = new BraveAccountBrowserProxyImpl())
  }
}

let instance: BraveAccountBrowserProxy | null = null
