// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { loadTimeData } from '../i18n_setup.js'

export interface BravePrivacyBrowserProxy {
  wasPushMessagingEnabledAtStartup: () => boolean
  wasWindowsRecallDisabledAtStartup: () => boolean

  isWindowsRecallDisabled: () => Promise<boolean>
  setWindowsRecallDisabled: (disabled: boolean) => void
}

export class BravePrivacyBrowserProxyImpl implements BravePrivacyBrowserProxy {
  wasPushMessagingEnabledAtStartup(): boolean {
    return loadTimeData.getBoolean('pushMessagingEnabledAtStartup')
  }

  wasWindowsRecallDisabledAtStartup(): boolean {
    return loadTimeData.getBoolean('windowsRecallDisabledAtStartup')
  }

  isWindowsRecallDisabled(): Promise<boolean> {
    return sendWithPromise('isWindowsRecallDisabled')
  }

  setWindowsRecallDisabled(disabled: boolean) {
    chrome.send('setWindowsRecallDisabled', [disabled])
  }

  static getInstance(): BravePrivacyBrowserProxyImpl {
    return instance || (instance = new BravePrivacyBrowserProxyImpl())
  }
}

let instance: BravePrivacyBrowserProxy | null = null
