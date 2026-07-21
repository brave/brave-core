/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ThemeColorPickerClientInterface } from 'chrome://resources/cr_components/theme_color_picker/theme_color_picker.mojom-webui.js'

import {
  WelcomeApi,
  createWelcomeApi,
  ChromeColor,
  ColorScheme,
  ImportDataStatus,
  Theme,
} from '../api/welcome_api'

const mockChromeColors: ChromeColor[] = [
  {
    name: 'Blue',
    seed: { value: 0xff0b57d0 },
    background: { value: 0xffd3e3fd },
    foreground: { value: 0xff0b57d0 },
    base: { value: 0xffc7c7c7 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Aqua',
    seed: { value: 0xff00639b },
    background: { value: 0xffc2e7ff },
    foreground: { value: 0xff00639b },
    base: { value: 0xffc7c7c7 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Green',
    seed: { value: 0xff146c2e },
    background: { value: 0xffc4eed0 },
    foreground: { value: 0xff146c2e },
    base: { value: 0xffc7c7c7 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Rose',
    seed: { value: 0xffbf0049 },
    background: { value: 0xffffd9e2 },
    foreground: { value: 0xffbf0049 },
    base: { value: 0xffc7c7c7 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Purple',
    seed: { value: 0xff6750a4 },
    background: { value: 0xffe9ddff },
    foreground: { value: 0xff6750a4 },
    base: { value: 0xffc7c7c7 },
    variant: 1 as ChromeColor['variant'],
  },
]

function createMockTheme(): Theme {
  return {
    hasBackgroundImage: false,
    hasThirdPartyTheme: false,
    backgroundImageMainColor: null,
    isDarkMode: false,
    seedColor: { value: 0 },
    seedColorHue: 0,
    backgroundColor: { value: 0xffffffff },
    foregroundColor: { value: 0xffe3e3e3 },
    colorPickerIconColor: { value: 0xff3c3c3c },
    colorsManagedByPolicy: false,
    isGreyBaseline: true,
    browserColorVariant: 1 as Theme['browserColorVariant'],
    followDeviceTheme: false,
  }
}

export function createWelcomeApiMock(): WelcomeApi {
  let onImportStatusChange: ((status: ImportDataStatus) => void) | null = null
  let theme = createMockTheme()
  let themeColorPickerClient: ThemeColorPickerClientInterface | null = null

  const pushTheme = (next: Partial<Theme>) => {
    theme = { ...theme, ...next }
    themeColorPickerClient?.setTheme(theme)
  }

  const api = createWelcomeApi({
    welcomePageHandler: {
      setWelcomePage(page) {},
      getColorScheme: async () => ({
        colorScheme: ColorScheme.kSystem,
      }),
      setColorScheme: async (colorScheme) => {},
      getVerticalTabsEnabled: async () => ({
        enabled: false,
      }),
      setVerticalTabsEnabled: async (enabled) => {},
    },
    bindWelcomePageHandler: (page) => {},
    themeColorPickerHandler: {
      getChromeColors: async () => ({ colors: mockChromeColors }),
      updateTheme: () => {
        themeColorPickerClient?.setTheme(theme)
      },
      setDefaultColor: () => {
        pushTheme({
          seedColor: { value: 0 },
          foregroundColor: null,
          isGreyBaseline: false,
        })
      },
      setGreyDefaultColor: () => {
        pushTheme({
          foregroundColor: { value: 0xffe3e3e3 },
          isGreyBaseline: true,
        })
      },
      setSeedColor: (seedColor, browserColorVariant) => {
        pushTheme({
          seedColor,
          browserColorVariant,
          foregroundColor: seedColor,
          isGreyBaseline: false,
        })
      },
      setSeedColorFromHue: () => {},
      removeBackgroundImage: () => {},
    },
    bindThemeColorPickerHandler: (client) => {
      themeColorPickerClient = client
    },
    messages: {
      async getDefaultBrowserInfo() {
        return {
          canBeDefault: true,
          isDefault: false,
          isDisabledByPolicy: false,
          isUnknownError: false,
        }
      },
      async getBrowserProfilesForImport() {
        return [
          {
            index: 0,
            name: 'Google Chrome Person 1',
            profileName: '',
            autofillFormData: true,
            extensions: true,
            favorites: true,
            history: true,
            passwords: true,
            search: true,
          },
          {
            index: 1,
            name: 'Google Chrome Person 2',
            profileName: '',
            autofillFormData: true,
            extensions: true,
            favorites: true,
            history: true,
            passwords: true,
            search: true,
          },
          {
            index: 2,
            name: 'Microsoft Edge',
            profileName: '',
            autofillFormData: true,
            extensions: true,
            favorites: true,
            history: true,
            passwords: true,
            search: true,
          },
        ]
      },
      setAsDefaultBrowser() {},
      importData(profileIndex, types) {
        if (!onImportStatusChange) {
          return
        }
        onImportStatusChange('in-progress')
        setTimeout(() => {
          if (onImportStatusChange) {
            onImportStatusChange('succeeded')
          }
        }, 2000)
      },
      addImportStatusListener(fn) {
        onImportStatusChange = fn
      },
    },
  })

  return api
}
