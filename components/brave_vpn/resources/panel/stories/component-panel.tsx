import { select } from '@storybook/addon-knobs'
import * as React from 'react'
import * as S from './style'

// Components
import MainPanel from '../components/main-panel'
import ErrorPanel from '../components/error-panel'
import SelectRegion from '../components/select-region'
import { ConnectionState } from '../types/connection_state'

const statusOptions = {
  'Disconnected': ConnectionState.DISCONNECTED,
  'Connecting': ConnectionState.CONNECTING,
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

export const _Main = () => {
  const [isOn, setIsOn] = React.useState(false)
  const handleToggleClick = () => setIsOn(state => !state)
  const handleSelectRegionButtonClick = () => {/**/}

  return (
    <S.PanelFrame>
      <MainPanel
        isOn={isOn}
        status={select('Current Status', statusOptions, ConnectionState.DISCONNECTED)}
        region='Tokyo'
        onToggleClick={handleToggleClick}
        onSelectRegionButtonClick={handleSelectRegionButtonClick}
      />
    </S.PanelFrame>
  )
}

export const _Error = () => {
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
        region='Tokyo'
      />
    </S.PanelFrame>
  )
}

export const _SelectLocation = () => {
  const onDone = () => {
    alert('Going back')
  }
  return (
    <S.PanelFrame>
      <SelectRegion onDone={onDone} />
    </S.PanelFrame>
  )
}
