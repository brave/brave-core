// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as Styles from './style'
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
    return getLocale(S.BRAVE_VPN_CONNECTED)
  }

  if (status === ConnectionState.CONNECTING) {
    return getLocale(S.BRAVE_VPN_CONNECTING)
  }

  if (status === ConnectionState.DISCONNECTING) {
    return getLocale(S.BRAVE_VPN_DISCONNECTING)
  }

  if (status === ConnectionState.DISCONNECTED) {
    return getLocale(S.BRAVE_VPN_DISCONNECTED)
  }

  if (status === ConnectionState.CONNECT_NOT_ALLOWED) {
    return getLocale(S.BRAVE_VPN_DISCONNECTED)
  }

  if (status === ConnectionState.CONNECT_FAILED) {
    return getLocale(S.BRAVE_VPN_CONNECTION_FAILED)
  }

  return null
}

const getStatusIndicator = (status: ConnectionState, icon: string) => {
  if (status === ConnectionState.CONNECTED) {
    return <Styles.ActiveIndicator name={icon} />
  }

  if (
    status === ConnectionState.CONNECTING ||
    status === ConnectionState.DISCONNECTING
  ) {
    return <Styles.LoadingIcon />
  }

  if (
    status === ConnectionState.DISCONNECTED ||
    status === ConnectionState.CONNECT_NOT_ALLOWED
  ) {
    return <Styles.InActiveIndicator name={icon} />
  }

  if (status === ConnectionState.CONNECT_FAILED) {
    return <Styles.FailedIndicator name={icon} />
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
    <Styles.Content>
      <Styles.StatusBox>
        {getStatusIndicator(status, icon)}
        <Styles.StatusLabel color={getStatusTextColor(status)}>
          {getStatusText(status)}
        </Styles.StatusLabel>
      </Styles.StatusBox>
      <ToggleButton
        onChange={handleToggleChange}
        checked={isOn}
        disabled={props.disabled}
      />
    </Styles.Content>
  )
}

export default Toggle
