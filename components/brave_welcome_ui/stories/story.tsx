// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import welcomeDarkTheme from '../theme/welcome-dark'
import welcomeLightTheme from '../theme/welcome-light'
import { withKnobs } from '@storybook/addon-knobs'

import './locale'

// Components
import SelectBrowser from '../components/select-browser'
import SelectProfile from '../components/select-profile'
import SelectTheme from '../components/select-theme'
import HelpImprove from '../components/help-improve'
import ImportInProgress from '../components/import-in-progress'
import SetupComplete from '../components/setup-complete'
import Welcome from '../components/welcome'

import DataContext from '../state/context'
import { ViewType, Scenes } from '../state/component_types'
import Background from '../components/background'
import HelpWDP from '../components/help-wdp'

const payload = [
  {
    'autofillFormData': false,
    'extensions': false,
    'favorites': true,
    'history': false,
    'index': 0,
    'name': 'Safari',
    'passwords': false,
    'payments': false,
    'profileName': '',
    'search': false,
    'browserType': 'Safari'
  },
  {
    'autofillFormData': false,
    'extensions': false,
    'favorites': true,
    'history': true,
    'index': 1,
    'name': 'Chrome Person 1',
    'passwords': true,
    'payments': false,
    'profileName': '',
    'search': false,
    'browserType': 'Google Chrome'
  },
  {
    'autofillFormData': false,
    'extensions': false,
    'favorites': true,
    'history': true,
    'index': 2,
    'name': 'Chrome Marc Jacobs',
    'passwords': true,
    'payments': false,
    'profileName': '',
    'search': false,
    'browserType': 'Google Chrome'
  },
  {
    'autofillFormData': false,
    'extensions': false,
    'favorites': false,
    'history': true,
    'index': 3,
    'name': 'Chrome Canary Person 1',
    'passwords': true,
    'payments': false,
    'profileName': '',
    'search': false,
    'browserType': 'Chrome Canary'
  },
  {
    'autofillFormData': false,
    'extensions': false,
    'favorites': false,
    'history': true,
    'index': 4,
    'name': 'Chromium Person 1',
    'passwords': true,
    'payments': false,
    'profileName': '',
    'search': false,
    'browserType': 'Chromium'
  }
]

export default {
  title: 'Welcome/Components',
  args: {
    darkTheme: welcomeDarkTheme,
    lightTheme: welcomeLightTheme
  },
  parameters: {
    layout: 'fullscreen'
  },
  decorators: [
    (Story: any) => {
      // mock data
      const [currentSelectedBrowser, setCurrentSelectedBrowser] =
        React.useState<string | undefined>('Chrome')
      const [scenes, setScenes] = React.useState<Scenes | undefined>(undefined)

      const incrementCount = () => {}

      const store = {
        setViewType: () => {},
        setCurrentSelectedBrowser,
        incrementCount,
        setScenes,
        currentSelectedBrowser,
        browserProfiles: payload,
        currentSelectedBrowserProfiles: payload.filter(
          (profile) => profile.browserType === currentSelectedBrowser
        ),
        viewType: ViewType.DefaultBrowser,
        scenes
      }

      return (
        <DataContext.Provider value={store}>
          <Story />
        </DataContext.Provider>
      )
    },
    withKnobs
  ]
}

export const _SelectBrowser = () => {
  return <SelectBrowser />
}

export const _SelectProfile = () => {
  return <SelectProfile />
}

export const _SelectTheme = () => {
  return <SelectTheme />
}

export const _HelpImprove = () => {
  return <HelpImprove />
}

export const _ImportInProgress = () => {
  return <ImportInProgress />
}

export const _SetupComplete = () => {
  return <SetupComplete />
}

export const _Welcome = () => {
  return <Welcome />
}

export const _Background = () => {
  return <Background static={true} />
}

export const _HelpWDP = () => {
  return <HelpWDP />
}
