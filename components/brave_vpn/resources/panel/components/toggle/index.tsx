import * as React from 'react'
import * as S from './style'

interface Props {
  isOn: boolean
  onClick: () => void
}

function Toggle (props: Props) {
  return (
    <S.Box
      type='button'
      role='switch'
      aria-checked={props.isOn}
      onClick={props.onClick}
      isActive={props.isOn}
    >
      <S.Circle isActive={props.isOn} />
    </S.Box>
  )
}

export default Toggle
