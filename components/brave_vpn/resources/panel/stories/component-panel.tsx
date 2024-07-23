// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { boolean, select } from '@storybook/addon-knobs'
import * as React from 'react'
import { createStore } from 'redux'
import { Provider as ReduxProvider } from 'react-redux'

// Components
import './locale'
import * as S from './style'
import { ConnectionState } from '../api/panel_browser_api'
import { mockRegionList } from './mock-data/region-list'
import ErrorPanel from '../components/error-panel'
import SelectRegionList from '../components/select-region-list'
import MainPanel from '../components/main-panel'
import SellPanel from '../components/sell-panel'
import SettingsPanel from '../components/settings-panel'
import LoadingPanel from '../components/loading-panel'
import ContactSupport from '../components/contact-support'
import PurchaseFailedPanel from '../components/purchase-failed-panel'
import './mock-data/api'
import '@brave/leo/tokens/css/variables.css'

export default {
  title: 'VPN/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  },
  decorators: [
    (Story: any) => {
      // We're not adding a reducer here because UI in storybook
      // shouldn't trigger any actions therefore shouldn't modify any state
      const store = createStore(state => state, {
        hasError: boolean('hasError', false),
        expired: boolean('expired', false),
        isSelectingRegion: false,
        connectionStatus: select('Current Status', ConnectionState, ConnectionState.DISCONNECTED),
        regions: mockRegionList,
        currentRegion: mockRegionList[2]
      })

      return (
        <ReduxProvider store={store}>
          <Story />
        </ReduxProvider>
      )
    }
  ]
}

export const _Main = () => {
  return (
    <S.PanelFrame>
      <MainPanel />
    </S.PanelFrame>
  )
}

export const _Error = () => {
  return (
    <S.PanelFrame>
      <ErrorPanel showContactSupport={() => {}} />
    </S.PanelFrame>
  )
}

export const _PurchaseFailed = () => {
  return (
    <S.PanelFrame>
      <PurchaseFailedPanel />
    </S.PanelFrame>
  )
}

export const _SelectLocation = () => {
  return (
    <S.PanelFrame>
      <SelectRegionList />
    </S.PanelFrame>
  )
}

export const _SellPanel = () => {
  return (
    <S.PanelFrame>
      <SellPanel />
    </S.PanelFrame>
  )
}

export const _SettingsPanel = () => {
  const closeSettingsPanel = () => {}
  return (
    <S.PanelFrame>
      <SettingsPanel closeSettingsPanel={closeSettingsPanel} showContactSupport={() => {}} />
    </S.PanelFrame>
  )
}

export const _LoadingPanel = () => {
  return (
    <S.PanelFrame>
      <LoadingPanel />
    </S.PanelFrame>
  )
}

export const _ContactSupportPanel = () => {
  return (
    <S.PanelFrame>
      <ContactSupport onCloseContactSupport={() => {}} />
    </S.PanelFrame>
  )
}
