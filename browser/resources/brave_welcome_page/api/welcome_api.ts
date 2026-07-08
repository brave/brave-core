/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  WelcomePageHandler,
  WelcomePageHandlerInterface,
} from 'gen/brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.m.js'

import { addWebUiListener, sendWithPromise } from 'chrome://resources/js/cr.js'
import { createInterfaceApi, state } from '$web-common/api'

// Type returned from requestDefaultBrowserState message.
export interface DefaultBrowserInfo {
  canBeDefault: boolean
  isDefault: boolean
  isDisabledByPolicy: boolean
  isUnknownError: boolean
}

// Import data type names returned from initializeImportDialog message.
export const importDataTypes = [
  'autofillFormData',
  'extensions',
  'favorites',
  'history',
  'passwords',
  'search',
] as const

export type ImportDataType = (typeof importDataTypes)[number]

// Type returned from initializeImportDialog message.
export interface BrowserProfile {
  index: number
  name: string
  profileName: string
  autofillFormData: boolean
  extensions: boolean
  favorites: boolean
  history: boolean
  passwords: boolean
  search: boolean
}

// The current import status.
export type ImportDataStatus = '' | 'in-progress' | 'succeeded'

interface ApiInit {
  welcomePageHandler: WelcomePageHandlerInterface
  messages: {
    getDefaultBrowserInfo: () => Promise<DefaultBrowserInfo>
    getBrowserProfilesForImport: () => Promise<BrowserProfile[]>
    setAsDefaultBrowser: () => void
    importData: (profileIndex: number, types: Set<ImportDataType>) => void
  }
}

function defaultInit(): ApiInit {
  return {
    welcomePageHandler: WelcomePageHandler.getRemote(),
    messages: {
      getDefaultBrowserInfo() {
        return sendWithPromise('requestDefaultBrowserState')
      },
      getBrowserProfilesForImport() {
        return sendWithPromise('initializeImportDialog')
      },
      setAsDefaultBrowser() {
        chrome.send('setAsDefaultBrowser')
      },
      importData(profileIndex, types) {
        chrome.send('importData', [profileIndex, importTypesToDict(types)])
      },
    },
  }
}

export function createWelcomeApi(init = defaultInit()) {
  const api = createInterfaceApi({
    endpoints: {
      getDefaultBrowserState: {
        query: () => init.messages.getDefaultBrowserInfo(),
      },
      getBrowserProfilesForImport: {
        query: () => init.messages.getBrowserProfilesForImport(),
      },
      importDataStatus: state<ImportDataStatus>(''),
    },

    actions: {
      setAsDefaultBrowser: init.messages.setAsDefaultBrowser,
      importData: init.messages.importData,
    },
  })

  addWebUiListener('brave-import-data-status-changed', (status: any) => {
    switch (String(status?.event ?? '')) {
      case 'ImportStarted':
        api.importDataStatus.update('in-progress')
        break
      case 'ImportEnded':
        if (api.importDataStatus.current() === 'in-progress') {
          api.importDataStatus.update('succeeded')
        }
        break
    }
  })

  return api
}

export type WelcomeApi = ReturnType<typeof createWelcomeApi>

function importTypesToDict(types: Set<ImportDataType>) {
  return {
    'import_dialog_autofill_form_data': types.has('autofillFormData'),
    'import_dialog_extensions': types.has('extensions'),
    'import_dialog_bookmarks': types.has('favorites'),
    'import_dialog_history': types.has('history'),
    'import_dialog_saved_passwords': types.has('passwords'),
    'import_dialog_search_engine': types.has('search'),
  }
}
