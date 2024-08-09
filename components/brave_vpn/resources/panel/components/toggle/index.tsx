// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import { getLocale } from '$web-common/locale'

import { ConnectionState } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import ToggleButton from '@brave/leo/react/toggle'
import { color } from '@brave/leo/tokens/css/variables'

function useIsOnSelector() {
  // We derive isOn from connectionStatus to be synchronized
  const connectionStatus = useSelector((state) => state.connectionStatus)
  const isConnectionActive = [
    ConnectionState.CONNECTED,
    ConnectionState.CONNECTING
  ].includes(connectionStatus)
  return isConnectionActive
}

const getStatusText = (status: ConnectionState) => {
  if (status === ConnectionState.CONNECTED) {
    return getLocale('braveVpnConnected')
  }

  if (status === ConnectionState.CONNECTING) {
    return getLocale('braveVpnConnecting')
  }

  if (status === ConnectionState.DISCONNECTING) {
    return getLocale('braveVpnDisconnecting')
  }

  if (status === ConnectionState.DISCONNECTED) {
    return getLocale('braveVpnDisconnected')
  }

  if (status === ConnectionState.CONNECT_NOT_ALLOWED) {
    return getLocale('braveVpnDisconnected')
  }

  if (status === ConnectionState.CONNECT_FAILED) {
    return getLocale('braveVpnConnectionFailed')
  }

  return null
}

const getStatusIndicator = (status: ConnectionState, icon: string) => {
  if (status === ConnectionState.CONNECTED) {
    return <S.ActiveIndicator name={icon} />
  }

  if (
    status === ConnectionState.CONNECTING ||
    status === ConnectionState.DISCONNECTING
  ) {
    return <S.LoadingIcon />
  }

  if (
    status === ConnectionState.DISCONNECTED ||
    status === ConnectionState.CONNECT_NOT_ALLOWED
  ) {
    return <S.InActiveIndicator name={icon} />
  }

  if (status === ConnectionState.CONNECT_FAILED) {
    return <S.FailedIndicator name={icon} />
  }

  return null
}

function getStatusTextColor(status: ConnectionState) {
  let statusTextColor: string = color.text.secondary
  if (status === ConnectionState.CONNECTED) {
    statusTextColor = color.systemfeedback.successText
  } else if (
    status === ConnectionState.CONNECTING ||
    status === ConnectionState.DISCONNECTING
  ) {
    statusTextColor = color.text.interactive
  }

  return statusTextColor
}

interface ToggleProps {
  disabled: boolean
}

function Toggle(props: ToggleProps) {
  const dispatch = useDispatch()
  const isOn = useIsOnSelector()
  const status = useSelector((state) => state.connectionStatus)

  const handleToggleChange = ({ checked }: { checked: boolean }) => {
    if (checked) dispatch(Actions.connect())
    else dispatch(Actions.disconnect())
  }

  let icon = 'remove-circle-filled'
  if (status === ConnectionState.CONNECTED) {
    icon = 'check-circle-filled'
  }

  return (
    <S.Content>
      <S.StatusBox>
        {getStatusIndicator(status, icon)}
        <S.StatusLabel color={getStatusTextColor(status)}>
          {getStatusText(status)}
        </S.StatusLabel>
      </S.StatusBox>
      <ToggleButton
        onChange={handleToggleChange}
        checked={isOn}
        disabled={props.disabled}
      />
    </S.Content>
  )
}

export default Toggle
