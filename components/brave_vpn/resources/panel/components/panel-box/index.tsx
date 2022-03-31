import * as React from 'react'
import * as S from './style'

interface Props {
  children: React.ReactNode
}

function PanelBox (props: Props) {
  return (
    <S.Box>
      {props.children}
      <S.Waves />
    </S.Box>
  )
}

export default PanelBox
