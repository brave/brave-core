import * as React from 'react'
import * as S from './style'
import locale from '../../constants/locale'
import { ConnectionState } from '../../types/connection_state'
import { LoaderIcon } from 'brave-ui/components/icons'
interface Props {
  status: ConnectionState
}

function StatusIndicator (props: Props) {
  return (
    <S.Box>
      {props.status === ConnectionState.CONNECTED && <S.ActiveIndicator />}
      {props.status === ConnectionState.CONNECTING && <S.Loader><LoaderIcon /></S.Loader>}
      {props.status === ConnectionState.DISCONNECTED && <S.InActiveIndicator />}
      <S.Text>
        {props.status === ConnectionState.CONNECTED && locale.connectedLabel}
        {props.status === ConnectionState.CONNECTING && `${locale.connectingLabel}...`}
        {props.status === ConnectionState.DISCONNECTED && locale.disconnectedLabel}
        {props.status === ConnectionState.CONNECT_FAILED && locale.connectionFailedLabel}
      </S.Text>
    </S.Box>
  )
}

export default StatusIndicator
