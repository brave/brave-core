// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { LoaderIcon } from 'brave-ui/components/icons'

import * as S from './style'
import { getLocale } from '../../../../../common/locale'

import { ConnectionState } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import { FabulouslyLargeToggle } from '$web-components/toggle'

function useIsOnSelector () {
  // We derive isOn from connectionStatus to be synchronized
  const connectionStatus = useSelector(state => state.connectionStatus)
  const isConnectionActive = [ConnectionState.CONNECTED, ConnectionState.CONNECTING].includes(connectionStatus)
  return isConnectionActive
}

interface ToggleProps {
  disabled: boolean
}

function Toggle (props: ToggleProps) {
  const dispatch = useDispatch()
  const isOn = useIsOnSelector()
  const status = useSelector(state => state.connectionStatus)

  const handleToggleChange = (isOn: boolean) => {
    if (isOn) dispatch(Actions.connect())
    else dispatch(Actions.disconnect())
  }

  return (
    <>
      <FabulouslyLargeToggle
        onChange={handleToggleChange}
        isOn={isOn}
        brand="vpn"
        disabled={props.disabled}
      />
      <S.StatusBox>
        {status === ConnectionState.CONNECTED && <S.ActiveIndicator />}
        {status === ConnectionState.CONNECTING && <S.Loader><LoaderIcon /></S.Loader>}
        {status === ConnectionState.DISCONNECTING && <S.Loader><LoaderIcon /></S.Loader>}
        {status === ConnectionState.DISCONNECTED && <S.InActiveIndicator />}
        {status === ConnectionState.CONNECT_NOT_ALLOWED && <S.InActiveIndicator />}
        {status === ConnectionState.CONNECT_FAILED && <S.FailedIndicator />}
        <S.StatusText>
          {status === ConnectionState.CONNECTED && getLocale('braveVpnConnected')}
          {status === ConnectionState.CONNECTING && getLocale('braveVpnConnecting')}
          {status === ConnectionState.DISCONNECTING && getLocale('braveVpnDisconnecting')}
          {status === ConnectionState.DISCONNECTED && getLocale('braveVpnDisconnected')}
          {status === ConnectionState.CONNECT_NOT_ALLOWED && getLocale('braveVpnDisconnected')}
          {status === ConnectionState.CONNECT_FAILED && getLocale('braveVpnConnectionFailed')}
        </S.StatusText>
      </S.StatusBox>
    </>
  )
}

export default Toggle
