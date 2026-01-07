/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  ColorScheme,
  WelcomePageHandler,
  WelcomePageInterface,
  WelcomePageReceiver,
  WelcomePageRemote,
  WelcomePageHandlerInterface,
} from 'gen/brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.m.js'

import {
  Theme,
  ChromeColor,
  ThemeColorPickerHandlerInterface,
  ThemeColorPickerClientInterface,
} from 'chrome://resources/cr_components/theme_color_picker/theme_color_picker.mojom-webui.js'

import { addWebUiListener, sendWithPromise } from 'chrome://resources/js/cr.js'
import { ThemeColorPickerProxy } from './theme_color_picker_proxy'
import {
  createInterfaceApi,
  endpointsFor,
  eventsFor,
  state,
} from '$web-common/api'

export { ColorScheme, Theme, ChromeColor }

export type SkColor = ChromeColor['seed']

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

export type ThemeColorPickerEventSource =
  EventSource<ThemeColorPickerClientInterface>

interface ApiInit {
  welcomePageHandler: WelcomePageHandlerInterface
  getWelcomePageRemote?: (page: WelcomePageInterface) => WelcomePageRemote
  themeColorPickerHandler: ThemeColorPickerHandlerInterface
  themeColorPickerEventSource: ThemeColorPickerEventSource
  messages: {
    getDefaultBrowserInfo: () => Promise<DefaultBrowserInfo>
    getBrowserProfilesForImport: () => Promise<BrowserProfile[]>
    setAsDefaultBrowser: () => void
    importData: (profileIndex: number, types: Set<ImportDataType>) => void
    addImportStatusListener: (fn: (status: ImportDataStatus) => void) => void
  }
}

function defaultInit(): ApiInit {
  const themeColorPickerProxy = ThemeColorPickerProxy.getInstance()
  return {
    welcomePageHandler: WelcomePageHandler.getRemote(),
    getWelcomePageRemote: (page) => {
      return new WelcomePageReceiver(page).$.bindNewPipeAndPassRemote()
    },
    themeColorPickerHandler: themeColorPickerProxy.handler,
    themeColorPickerEventSource: eventSourceFromRouter(
      themeColorPickerProxy.callbackRouter,
    ),
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
      addImportStatusListener(fn) {
        addWebUiListener('brave-import-data-status-changed', (status: any) => {
          switch (String(status?.event ?? '')) {
            case 'ImportStarted':
              fn('in-progress')
              break
            case 'ImportEnded':
              fn('succeeded')
              break
          }
        })
      },
    },
  }
}

export function createWelcomeApi(init = defaultInit()) {
  const {
    welcomePageHandler,
    themeColorPickerHandler,
    themeColorPickerEventSource,
  } = init

  const api = createInterfaceApi({
    endpoints: {
      ...endpointsFor(themeColorPickerHandler, {
        getChromeColors: {
          response: (r) => r.colors,
          placeholderData: [] as ChromeColor[],
        },
      }),
      theme: state<Theme | undefined>(),
      ...endpointsFor(welcomePageHandler, {
        getColorScheme: {
          response: (r) => r.colorScheme,
          prefetchWithArgs: [],
          placeholderData: ColorScheme.kSystem,
        },
        setColorScheme: {
          mutationResponse: () => {},
          onMutate: ([colorScheme]: [ColorScheme]) => {
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
        query: () => init.messages.getDefaultBrowserInfo(),
      },
      getBrowserProfilesForImport: {
        query: () => init.messages.getBrowserProfilesForImport(),
      },
      importDataStatus: state<ImportDataStatus>(''),
    },

    events: {
      ...eventsFor(
        WelcomePageInterface,
        {
          onColorSchemeChanged() {
            api.getColorScheme.invalidate()
          },
          onVerticalTabsEnabledChanged() {
            api.getVerticalTabsEnabled.invalidate()
          },
        },
        (observer) => {
          if (init.getWelcomePageRemote) {
            welcomePageHandler.setWelcomePage(
              init.getWelcomePageRemote(observer),
            )
          }
        },
      ),
    },

    actions: {
      setAsDefaultBrowser: init.messages.setAsDefaultBrowser,
      importData: init.messages.importData,
      setThemeColor: (
        seed: ChromeColor['seed'],
        variant: ChromeColor['variant'],
      ) => {
        themeColorPickerHandler.setSeedColor(seed, variant)
      },
      setGreyThemeColor: () => {
        themeColorPickerHandler.setGreyDefaultColor()
      },
    },
  })

  themeColorPickerEventSource.addListeners({
    setTheme(theme) {
      api.theme.update(theme)
    },
  })

  themeColorPickerHandler.updateTheme()

  init.messages.addImportStatusListener((status) => {
    switch (status) {
      case 'in-progress':
        api.importDataStatus.update(status)
        break
      case 'succeeded':
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
