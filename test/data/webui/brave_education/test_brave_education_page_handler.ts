/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import type { PageHandlerInterface } from 'chrome://getting-started/brave_education.mojom-webui.js'
import { TestBrowserProxy } from 'chrome://webui-test/test_browser_proxy.js'

export class TestBraveEducationPageHandler
  extends TestBrowserProxy
  implements PageHandlerInterface
{
  private url_ = ''

  constructor() {
    super(['getServerUrl'])
  }

  setServerUrl(url: string) {
    this.url_ = url
  }

  getServerUrl() {
    this.methodCalled('getServerUrl')
    return Promise.resolve({ url: this.url_ })
  }
}
