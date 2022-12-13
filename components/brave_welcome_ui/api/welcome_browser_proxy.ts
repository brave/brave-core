// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { DefaultBrowserBrowserProxyImpl } from './default_browser_browser_proxy'
import { ImportDataBrowserProxyImpl, BrowserProfile as _BrowserProfile } from './import_data_browser_proxy'

interface P3APayload {
  currentScreen: number
  isFinished: boolean
  isSkipped: boolean
}

export interface BrowserProfile extends _BrowserProfile {
  browserType?: string | undefined
}

export const defaultImportTypes = {
  import_dialog_autofill_form_data: true,
  import_dialog_bookmarks: true,
  import_dialog_history: true,
  import_dialog_saved_passwords: true,
  import_dialog_search_engine: true,
  import_dialog_extensions: true,
  import_dialog_payments: true
}

export interface WelcomeBrowserProxy {
  recordP3A: (payload: P3APayload) => void
  setP3AEnabled: (enabled: boolean) => void
  setMetricsReportingEnabled: (enabled: boolean) => void
  openSettingsPage: () => void
}

export { DefaultBrowserBrowserProxyImpl, ImportDataBrowserProxyImpl }

export class WelcomeBrowserProxyImpl implements WelcomeBrowserProxy {
  recordP3A (payload: P3APayload) {
    chrome.send('recordP3A', [payload.currentScreen, payload.isFinished, payload.isSkipped])
  }

  setP3AEnabled (enabled: boolean) {
    chrome.send('setP3AEnabled', [enabled])
  }

  setMetricsReportingEnabled (enabled: boolean) {
    chrome.send('setMetricsReportingEnabled', [enabled])
  }

  openSettingsPage () {
    chrome.send('openSettingsPage')
  }

  static getInstance (): WelcomeBrowserProxy {
    return instance || (instance = new WelcomeBrowserProxyImpl())
  }

  static setInstance (obj: WelcomeBrowserProxy) {
    instance = obj
  }
}

let instance: WelcomeBrowserProxyImpl|null = null
