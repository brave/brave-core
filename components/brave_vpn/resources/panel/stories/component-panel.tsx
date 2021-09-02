import * as React from 'react'
import * as S from './style'

// Components
import Main from '../containers/main'
import SelectRegion from '../components/select-region'

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
  return (
    <S.PanelFrame>
      <Main />
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
