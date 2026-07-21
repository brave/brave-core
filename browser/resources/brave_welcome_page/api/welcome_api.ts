/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  ColorScheme,
  WelcomePageHandler,
  WelcomePageInterface,
  WelcomePageReceiver,
  WelcomePageHandlerInterface,
} from 'gen/brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.m.js'

import {
  Theme,
  ChromeColor,
  ThemeColorPickerClientInterface,
  ThemeColorPickerClientReceiver,
  ThemeColorPickerHandlerFactory,
  ThemeColorPickerHandlerInterface,
  ThemeColorPickerHandlerRemote,
} from 'chrome://resources/cr_components/theme_color_picker/theme_color_picker.mojom-webui.js'

import { addWebUiListener, sendWithPromise } from 'chrome://resources/js/cr.js'
import { createInterfaceApi, endpointsFor, state } from '$web-common/api'

export { ColorScheme, Theme, ChromeColor }

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
  bindWelcomePageHandler: (page: WelcomePageInterface) => void
  themeColorPickerHandler: ThemeColorPickerHandlerInterface
  bindThemeColorPickerHandler: (client: ThemeColorPickerClientInterface) => void
  messages: {
    getDefaultBrowserInfo: () => Promise<DefaultBrowserInfo>
    getBrowserProfilesForImport: () => Promise<BrowserProfile[]>
    setAsDefaultBrowser: () => void
    importData: (profileIndex: number, types: Set<ImportDataType>) => void
    addImportStatusListener: (fn: (status: ImportDataStatus) => void) => void
  }
}

function defaultInit(): ApiInit {
  const welcomePageHandler = WelcomePageHandler.getRemote()
  const themeColorPickerHandler = new ThemeColorPickerHandlerRemote()
  return {
    welcomePageHandler,
    bindWelcomePageHandler: (page) => {
      welcomePageHandler.setWelcomePage(
        new WelcomePageReceiver(page).$.bindNewPipeAndPassRemote(),
      )
    },
    themeColorPickerHandler,
    bindThemeColorPickerHandler: (client) => {
      ThemeColorPickerHandlerFactory.getRemote().createThemeColorPickerHandler(
        new ThemeColorPickerClientReceiver(client).$.bindNewPipeAndPassRemote(),
        themeColorPickerHandler.$.bindNewPipeAndPassReceiver(),
      )
    },
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
  const { welcomePageHandler, themeColorPickerHandler } = init

  const api = createInterfaceApi({
    endpoints: {
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
      ...endpointsFor(themeColorPickerHandler, {
        getChromeColors: {
          response: (r) => r.colors,
          placeholderData: [] as ChromeColor[],
        },
      }),
      theme: state<Theme | undefined>(),
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

  init.bindWelcomePageHandler({
    onThemeChanged: () => {
      api.getColorScheme.invalidate()
    },
    onVerticalTabsEnabledChanged: () => {
      api.getVerticalTabsEnabled.invalidate()
    },
  })

  init.bindThemeColorPickerHandler({
    setTheme: (theme) => {
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
