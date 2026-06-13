/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { addWebUiListener, sendWithPromise } from 'chrome://resources/js/cr.js'
import { createInterfaceApi, endpointsFor, state } from '$web-common/api'
import { WelcomePageProxy, mojom } from './welcome_page_proxy'

export { mojom }

export const ColorScheme = mojom.ColorScheme

export enum P3APhase {
  Welcome = 0,
  Import = 1,
  Consent = 2,
  Finished = 3,
}

export interface DefaultBrowserInfo {
  canBeDefault: boolean
  isDefault: boolean
  isDisabledByPolicy: boolean
  isUnknownError: boolean
}

export const importDataTypes = [
  'autofillFormData',
  'extensions',
  'favorites',
  'history',
  'passwords',
  'search',
] as const

export type ImportDataType = (typeof importDataTypes)[number]

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

export type ImportDataStatus = '' | 'inProgress' | 'succeeded' | 'failed'

interface EventSource<T> {
  addListeners: (listeners: Partial<T>) => () => void
}

function eventSourceFromRouter<T>(router: any): EventSource<T> {
  return {
    addListeners(listeners) {
      const ids: number[] = []
      for (const [key, val] of Object.entries(listeners)) {
        ids.push(router[key].addListener(val))
      }
      return () => {
        for (const id of ids) {
          router.removeListener(id)
        }
      }
    },
  }
}

export type WelcomePageEventSource = EventSource<mojom.WelcomePageInterface>

interface ApiInit {
  welcomePageHandler: mojom.WelcomePageHandlerInterface
  welcomePageEventSource: WelcomePageEventSource
}

function defaultInit(): ApiInit {
  const proxy = WelcomePageProxy.getInstance()
  return {
    welcomePageHandler: proxy.handler,
    welcomePageEventSource: eventSourceFromRouter(proxy.callbackRouter),
  }
}

export function createWelcomeApi(init = defaultInit()) {
  const { welcomePageHandler, welcomePageEventSource } = init

  const api = createInterfaceApi({
    endpoints: {
      ...endpointsFor(welcomePageHandler, {
        getColorScheme: {
          response: (r) => r.colorScheme,
          prefetchWithArgs: [],
          placeholderData: mojom.ColorScheme.kSystem,
        },
        setColorScheme: {
          mutationResponse: () => {},
          onMutate: ([colorScheme]: [mojom.ColorScheme]) => {
            api.getColorScheme.update(colorScheme)
          },
        },
        getVerticalTabsEnabled: {
          response: (r) => r.enabled,
          prefetchWithArgs: [],
          placeholderData: false,
        },
        setVerticalTabsEnabled: {
          mutationResponse: () => {},
          onMutate: ([enabled]: [boolean]) => {
            api.getVerticalTabsEnabled.update(enabled)
          },
        },
      }),
      getDefaultBrowserState: {
        query: () =>
          sendWithPromise<DefaultBrowserInfo>('requestDefaultBrowserState'),
      },
      getBrowserProfilesForImport: {
        query: () =>
          sendWithPromise<BrowserProfile[]>('initializeImportDialog'),
      },
      importDataStatus: state<ImportDataStatus>(''),
    },

    actions: {
      setAsDefaultBrowser: () => chrome.send('setAsDefaultBrowser'),

      importData: (profileIndex: number, types: Set<ImportDataType>) =>
        chrome.send('importData', [profileIndex, importTypesToDict(types)]),
    },
  })

  welcomePageEventSource.addListeners({
    onColorSchemeChanged() {
      api.getColorScheme.invalidate()
    },
    onVerticalTabsEnabledChanged() {
      api.getVerticalTabsEnabled.invalidate()
    },
  })

  addWebUiListener('import-data-status-changed', (status: unknown) => {
    switch (status) {
      case 'inProgress':
      case 'failed':
      case 'succeeded':
        api.importDataStatus.update(status)
        break
      default:
        api.importDataStatus.update('')
        break
    }
  })

  // TODO: Possibly listen to 'brave-import-data-status-changed'. It may give
  // more granular data.

  return api
}

export type WelcomeApi = ReturnType<typeof createWelcomeApi>

function importTypesToDict(types: Set<ImportDataType>) {
  let dict: any = {}
  for (const type of types) {
    dict[importTypeToDialogKey(type)] = true
  }
  return dict
}

function importTypeToDialogKey(type: ImportDataType) {
  switch (type) {
    case 'autofillFormData':
      return 'import_dialog_autofill_form_data'
    case 'extensions':
      return 'import_dialog_extensions'
    case 'favorites':
      return 'import_dialog_bookmarks'
    case 'history':
      return 'import_dialog_history'
    case 'passwords':
      return 'import_dialog_saved_passwords'
    case 'search':
      return 'import_dialog_search_engine'
  }
}
