import { select } from '@storybook/addon-knobs'
import * as React from 'react'
import { createStore } from 'redux'
import { Provider as ReduxProvider } from 'react-redux'
// Components
import * as S from './style'
import { ConnectionState } from '../api/panel_browser_api'
import { mockRegionList } from './mock-data/region-list'
import ErrorPanel from '../components/error-panel'
import SelectRegionList from '../components/select-region-list'
import MainPanel from '../components/main-panel'

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
        hasError: false,
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
      <ErrorPanel />
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
