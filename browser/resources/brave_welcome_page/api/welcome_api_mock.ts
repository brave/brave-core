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
  FeatureVisibility,
  featureVisibilityKeys,
  ImportDataStatus,
  Theme,
} from '../api/welcome_api'

// Mirrors the names, seeds and variants of `kDynamicCustomizeChromeColors`.
const mockChromeColors: ChromeColor[] = [
  {
    name: 'Blue',
    seed: { value: 0xff8cabe4 },
    background: { value: 0xff2753a5 },
    foreground: { value: 0xffd6e1f5 },
    base: { value: 0xffacc3ec },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Cool grey',
    seed: { value: 0xff8cabe4 },
    background: { value: 0xff596273 },
    foreground: { value: 0xffe2e5e9 },
    base: { value: 0xffc6cad2 },
    variant: 2 as ChromeColor['variant'],
  },
  {
    name: 'Grey',
    seed: { value: 0xff888888 },
    background: { value: 0xff666666 },
    foreground: { value: 0xffe6e6e6 },
    base: { value: 0xffcccccc },
    variant: 2 as ChromeColor['variant'],
  },
  {
    name: 'Aqua',
    seed: { value: 0xff26a69a },
    background: { value: 0xff26a69a },
    foreground: { value: 0xffd6f6f3 },
    base: { value: 0xffacece6 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Green',
    seed: { value: 0xff00ff00 },
    background: { value: 0xff00cc00 },
    foreground: { value: 0xffccffcc },
    base: { value: 0xff99ff99 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Viridian',
    seed: { value: 0xff87ba81 },
    background: { value: 0xff616c60 },
    foreground: { value: 0xffe4e7e4 },
    base: { value: 0xffcacfc9 },
    variant: 2 as ChromeColor['variant'],
  },
  {
    name: 'Citron',
    seed: { value: 0xfffadf73 },
    background: { value: 0xffc59f07 },
    foreground: { value: 0xfffdf4ce },
    base: { value: 0xfffbe89d },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Orange',
    seed: { value: 0xffff8000 },
    background: { value: 0xffcc6600 },
    foreground: { value: 0xffffe6cc },
    base: { value: 0xffffcc99 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Apricot',
    seed: { value: 0xfffcdbc9 },
    background: { value: 0xff786154 },
    foreground: { value: 0xffeae4e1 },
    base: { value: 0xffd5c9c3 },
    variant: 2 as ChromeColor['variant'],
  },
  {
    name: 'Rose',
    seed: { value: 0xfff3b2be },
    background: { value: 0xffb01c37 },
    foreground: { value: 0xfff8d3da },
    base: { value: 0xfff1a7b5 },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Pink',
    seed: { value: 0xfff3b2be },
    background: { value: 0xff75575d },
    foreground: { value: 0xffe9e2e3 },
    base: { value: 0xffd3c5c7 },
    variant: 2 as ChromeColor['variant'],
  },
  {
    name: 'Fuchsia',
    seed: { value: 0xffff00ff },
    background: { value: 0xffcc00cc },
    foreground: { value: 0xffffccff },
    base: { value: 0xffff99ff },
    variant: 1 as ChromeColor['variant'],
  },
  {
    name: 'Violet',
    seed: { value: 0xffe5d5fc },
    background: { value: 0xff560ebe },
    foreground: { value: 0xffe2cffc },
    base: { value: 0xffc4a0f8 },
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

  const featureVisibility: FeatureVisibility = {
    aiChat: true,
    wallet: true,
    rewards: true,
    vpn: true,
  }

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
      getFeatureVisibility: async () => ({
        visibility: { ...featureVisibility },
      }),
      setFeatureVisible: async (feature, visible) => {
        featureVisibility[featureVisibilityKeys[feature]] = visible
      },
      setCrashReportsEnabled: async (enabled) => {},
      setP3AEnabled: async (enabled) => {},
      setWebDiscoveryEnabled: async (enabled) => {},
      getWelcomeCompleteURL: async () => ({ url: 'chrome://newtab' }),
      setOnboardingPhase: (phase) => {},
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
    isCrashReportingPrefManaged: false,
    isP3APrefManaged: false,
    isWebDiscoveryPrefManaged: false,
    webDiscoveryFeatureEnabled: true,
    aiChatFeatureEnabled: true,
    walletFeatureEnabled: true,
    rewardsFeatureEnabled: true,
    vpnFeatureEnabled: true,
  })

  return api
}
