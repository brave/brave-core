import * as React from 'react'
import * as S from './style'
import locale from '../../constants/locale'
interface Props {
  isConnected: boolean
}

function StatusIndicator (props: Props) {
  return (
    <S.Box>
      <S.Indicator isActive={props.isConnected} />
      <S.Text>
        {props.isConnected
          ? locale.connectedLabel
          : locale.disconnectedLabel}
      </S.Text>
    </S.Box>
  )
}

export default StatusIndicator
