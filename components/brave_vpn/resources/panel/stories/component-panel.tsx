import { select } from '@storybook/addon-knobs'
import * as React from 'react'
import * as S from './style'

// Components
import { mockRegionList } from './mock-data/region-list'
import MainPanel from '../components/main-panel'
import ErrorPanel from '../components/error-panel'
import SelectRegionList from '../components/select-region-list'
import { RegionState, Region } from '../api/region_interface'
import { ConnectionState } from '../api/panel_browser_api'

const statusOptions = {
  'Disconnected': ConnectionState.DISCONNECTED,
  'Connecting': ConnectionState.CONNECTING,
  'Disconnecting': ConnectionState.DISCONNECTING,
  'Connected': ConnectionState.CONNECTED
}

export default {
  title: 'VPN/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
}

function useRegion () {
  const result = React.useState<RegionState>(mockRegionList)
  return result
}

export const _Main = () => {
  const [isOn, setIsOn] = React.useState(false)
  const handleToggleClick = () => setIsOn(state => !state)
  const handleSelectRegionButtonClick = () => {/**/}
  const [region] = useRegion()

  if (!region.current) return null

  return (
    <S.PanelFrame>
      <MainPanel
        isOn={isOn}
        status={select('Current Status', statusOptions, ConnectionState.DISCONNECTED)}
        region={region.current}
        onToggleClick={handleToggleClick}
        onSelectRegionButtonClick={handleSelectRegionButtonClick}
      />
    </S.PanelFrame>
  )
}

export const _Error = () => {
  const [region] = useRegion()

  const handleTryAgain = () => {
    alert('Trying..')
  }

  const handleChooseServer = () => {
    alert('Server selection panel')
  }

  return (
    <S.PanelFrame>
      <ErrorPanel
        onTryAgainClick={handleTryAgain}
        onChooseServerClick={handleChooseServer}
        region={region}
      />
    </S.PanelFrame>
  )
}

export const _SelectLocation = () => {
  const [region, setRegion] = useRegion()

  const onDone = () => {
    alert('Going back')
  }

  const clickConnect = () => {
    alert('Connecting...')
  }

  const handleRegionClick = (currentRegion: Region) => {
    setRegion((prevState) => ({
      ...prevState,
      current: currentRegion
    }))
  }

  return (
    <S.PanelFrame>
      <SelectRegionList
        onDone={onDone}
        regions={region.all!}
        selectedRegion={region.current!}
        onRegionClick={handleRegionClick}
        onConnectClick={clickConnect}
      />
    </S.PanelFrame>
  )
}
