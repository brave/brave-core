import * as React from 'react'
import { LoaderIcon } from 'brave-ui/components/icons'

import * as S from './style'
import { getLocale } from '../../../../../common/locale'

import { ConnectionState } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'

function useIsOnSelector () {
  // We derive isOn from connectionStatus to be synchronized
  const connectionStatus = useSelector(state => state.connectionStatus)
  const isConnectionActive = [ConnectionState.CONNECTED, ConnectionState.CONNECTING].includes(connectionStatus)
  return isConnectionActive
}

function Toggle () {
  const dispatch = useDispatch()
  const isOn = useIsOnSelector()
  const status = useSelector(state => state.connectionStatus)

  const onToggleClick = () => {
    const activated = !isOn
    if (activated) dispatch(Actions.connect())
    else dispatch(Actions.disconnect())
  }

  return (
    <>
      <S.ToggleBox
        type='button'
        role='switch'
        aria-checked={isOn}
        onClick={onToggleClick}
        isActive={isOn}
      >
        <S.Knob isActive={isOn} />
      </S.ToggleBox>
      <S.StatusBox>
        {status === ConnectionState.CONNECTED && <S.ActiveIndicator />}
        {status === ConnectionState.CONNECTING && <S.Loader><LoaderIcon /></S.Loader>}
        {status === ConnectionState.DISCONNECTING && <S.Loader><LoaderIcon /></S.Loader>}
        {status === ConnectionState.DISCONNECTED && <S.InActiveIndicator />}
        {status === ConnectionState.CONNECT_FAILED && <S.FailedIndicator />}
        <S.StatusText>
          {status === ConnectionState.CONNECTED && getLocale('braveVpnConnected')}
          {status === ConnectionState.CONNECTING && getLocale('braveVpnConnecting')}
          {status === ConnectionState.DISCONNECTING && getLocale('braveVpnDisconnecting')}
          {status === ConnectionState.DISCONNECTED && getLocale('braveVpnDisconnected')}
          {status === ConnectionState.CONNECT_FAILED && getLocale('braveVpnConnectionFailed')}
        </S.StatusText>
      </S.StatusBox>
    </>
  )
}

export default Toggle
