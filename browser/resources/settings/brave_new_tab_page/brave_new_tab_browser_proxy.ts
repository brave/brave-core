/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {sendWithPromise} from 'chrome://resources/js/cr.js';

export type NewTabOption = {
  name: string
  value: number // corresponds to NewTabShowsOption enum
}

export interface BraveNewTabBrowserProxy {
  getNewTabShowsOptionsList(): Promise<NewTabOption[]>
  shouldShowNewTabDashboardSettings(): Promise<boolean>
}

export class BraveNewTabBrowserProxyImpl implements BraveNewTabBrowserProxy {
  getNewTabShowsOptionsList() {
    return sendWithPromise('getNewTabShowsOptionsList')
  }

  shouldShowNewTabDashboardSettings() {
    return sendWithPromise('shouldShowNewTabDashboardSettings')
  }

  static getInstance(): BraveNewTabBrowserProxyImpl {
    return instance || (instance = new BraveNewTabBrowserProxyImpl())
  }
}

let instance: BraveNewTabBrowserProxyImpl|null = null
