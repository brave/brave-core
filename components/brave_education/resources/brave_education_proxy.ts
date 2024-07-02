/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { sendWithPromise } from 'chrome://resources/js/cr.js'

let instance: BraveEducationProxy | null = null;

export class BraveEducationProxy {
  static getInstance (): BraveEducationProxy {
    return instance || (instance = new BraveEducationProxy());
  }

  initialize (pageURL: string): Promise<string> {
    return sendWithPromise('initialize', pageURL);
  }
}
