/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
export {}

type loadTimeData = {
  getString: (key: string) => string
  data_: Record<string, string>
}

declare global {
  interface Window {
    loadTimeData: loadTimeData
    cr: {
      define: (name: string, init: () => void) => void
    }
    i18nTemplate: {
      process: (document: Document, translations: loadTimeData) => void
    }
    brave_adblock: {
      initialize: () => void
    }
    brave_new_tab: {
      initialize: () => void
      statsUpdated: () => void
    }
    brave_rewards: {
      initialize: () => void
      walletCreated: () => void
      walletCreateFailed: () => void
    }
    brave_welcome: {
      initialize: () => void
    },
    sync_ui_exports: {
      initialize: () => void
    }
  }
}
