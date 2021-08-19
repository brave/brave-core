import * as React from 'react'
import * as S from './style'

// Components
import Main from '../components/main'

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
    <S.StyledExtensionWrapper>
      <Main />
    </S.StyledExtensionWrapper>
  )
}
