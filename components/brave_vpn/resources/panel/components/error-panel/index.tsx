// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as Styles from './style'
import { PanelHeader } from '../select-region-list'
import * as Actions from '../../state/actions'
import { useSelector, useDispatch } from '../../state/hooks'
import { getLocale } from '$web-common/locale'

interface Props {
  showContactSupport: () => void
  isAgentUnavailable?: boolean
}

function ErrorPanel(props: Props) {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const isReconnecting = useSelector((state) => state.isReconnectingToAgent)

  const handleShowMainView = () => {
    dispatch(Actions.resetConnectionState())
  }

  const handleTryAgain = () => {
    dispatch(props.isAgentUnavailable
      ? Actions.reconnectToAgent()
      : Actions.connect())
  }

  const handleChooseServer = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  const title = props.isAgentUnavailable
    ? getLocale(S.BRAVE_VPN_CONNECTION_UNAVAILABLE)
    : getLocale(S.BRAVE_VPN_UNABLE_CONNECT_TO_SERVER)

  const descriptionTemplate = props.isAgentUnavailable
    ? getLocale(S.BRAVE_VPN_CONNECT_NO_AGENT)
    : getLocale(S.BRAVE_VPN_UNABLE_CONNECT_INFO)

  const matches = {
    $1: getLocale(S.BRAVE_VPN),
    $2: currentRegion?.namePretty || ''
  }

  const description = descriptionTemplate.replace(
    /\$\d+/g,
    (match) => matches[match as keyof typeof matches]
  )

  return (
    <Styles.Box>
      <Styles.PanelContent>
        <PanelHeader
          title={getLocale(S.BRAVE_VPN)}
          buttonAriaLabel={getLocale(S.BRAVE_VPN_SUPPORT_PANEL_BACK_BUTTON_ARIA_LABEL)}
          onClick={props.isAgentUnavailable ? undefined : handleShowMainView}
        />
        <Styles.TopContent>
          <Styles.StyledAlert
            type='error'
            hideIcon
          >
            <div slot='title'>{title}</div>
            {description}
          </Styles.StyledAlert>
          <Styles.StyledActionButton
            slot='actions'
            kind='filled'
            onClick={handleTryAgain}
            isLoading={isReconnecting}
            isDisabled={isReconnecting}
          >
            {getLocale(S.BRAVE_VPN_TRY_AGAIN)}
          </Styles.StyledActionButton>
          {!props.isAgentUnavailable && (
            <Styles.StyledActionButton
              slot='actions'
              kind='plain'
              onClick={handleChooseServer}
            >
              {getLocale(S.BRAVE_VPN_CHOOSE_ANOTHER_SERVER)}
            </Styles.StyledActionButton>
          )}
          <Styles.StyledActionButton
            slot='actions'
            kind='plain'
            onClick={props.showContactSupport}
          >
            {getLocale(S.BRAVE_VPN_CONTACT_SUPPORT)}
          </Styles.StyledActionButton>
        </Styles.TopContent>
      </Styles.PanelContent>
    </Styles.Box>
  )
}

export default ErrorPanel
