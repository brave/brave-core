// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import { getLocale } from '$web-common/locale'
import getPageHandlerInstance, { ConnectionStatus } from '../../api/brave_page_handler'

interface BoxProps {
  isConnected?: boolean
  isLoading?: boolean
}
const Box = styled.div<BoxProps>`
  --bg-color: #BD1531;

  display: inline-flex;
  flex-direction: column;
  background: var(--bg-color);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  border-radius: 4px;
  padding: 8px 16px;
  max-width: 284px;

  ${p => p.isLoading && css`
    --bg-color: rgba(255, 255, 255, 0.15);
  `}

  ${p => p.isConnected && css`
    --bg-color: #12A378;
  `}

`

const Row = styled.div`
  display: inline-flex;
  flex-direction: row;
  align-items: center;
  gap: 12px;
  color: white;
  font-weight: 600;
  font-size: 14px;

  span {
    width: 22px;
    height: 22px;
    min-width: 22px;
  }

`

interface HelpProps {
  isVisible?: boolean
}

const Help = styled.div<HelpProps>`
  --display: none;

  ${p => p.isVisible && css`
    --display: inline-flex;
  `}
  
  display: var(--display);
  flex-direction: row;
  align-items: center;
  gap: 12px;
  color: white;
  font-weight: 400;
  font-size: 12px;

  span {
    width: 22px;
    height: 22px;
    min-width: 22px;
  }

  a {
    color: white;
    cursor: pointer;
    text-decoration-line: underline;
  }

`

interface Props {
  isConnected: boolean
  progress: string
  message: string
  connectionStatus: ConnectionStatus
}

function splitMessage (key: string) {
  return getLocale(key).split(/\$\d+/g)
}

function contactBraveSupport () {
  getPageHandlerInstance().pageHandler.goToBraveSupport()
}

function BadgeTor (props: Props) {
  let textElement = getLocale('torStatusDisconnected')
  let helpElement
  let iconElement = (
    <svg width="20" height="20" xmlns="http://www.w3.org/2000/svg">
      <path fillRule="evenodd" clipRule="evenodd" d="M10.02 18.181V16.97a6.97 6.97 0 0 0 0-13.938V1.818a8.181 8.181 0 0 1 0 16.363Zm0-4.243a3.94 3.94 0 0 0 0-7.877V4.85a5.15 5.15 0 0 1 0 10.301v-1.212Zm0-6.058a2.12 2.12 0 0 1 0 4.24V7.88ZM0 10c0 5.523 4.477 10 10 10s10-4.477 10-10S15.523 0 10 0 0 4.477 0 10Z" fill="#fff" />
    </svg>
  )

  const isLoading = props.connectionStatus === ConnectionStatus.kConnecting || props.connectionStatus === ConnectionStatus.kConnectionSlow

  if (props.isConnected || props.connectionStatus === ConnectionStatus.kConnected) {
    textElement = getLocale('torStatusConnected')
  } else if (props.connectionStatus === ConnectionStatus.kConnectionStuck) {
    textElement = getLocale('torStatusConnectionFailed')

    const [
      open,
      settings,
      reenable
    ] = splitMessage('torHelpDisconnectedReenable')

    const [
      before,
      havingTrouble,
      tryUsing,
      bridges,
      or,
      contactSupport,
      rest
    ] = splitMessage('torHelpDisconnectedBridges')

    helpElement = (<p>
      {open}<a href="chrome://settings/privacy">{settings}</a>{reenable}
      <br></br> <br></br>
      {before}<strong>{havingTrouble}</strong>
      {tryUsing}<a href="chrome://settings/privacy">{bridges}</a>
      {or}
      <a onClick={contactBraveSupport}>{contactSupport}</a>
      {rest}
    </p>)
  } else if (props.connectionStatus === ConnectionStatus.kConnectionSlow) {
    textElement = getLocale('torStatusConnectionSlow')
    helpElement = getLocale('torStatusConnectionSlowDesc')
    iconElement = <LoaderIcon />
  } else if (isLoading) {
    textElement = getLocale('torStatusInitializing', { percentage: props.progress })
    helpElement = props.message
    iconElement = <LoaderIcon />
  }

  return (
    <Box
      isLoading={isLoading}
      isConnected={props.isConnected}
    >
      <Row>
        <span>{iconElement}</span>
        {textElement}
      </Row>
      <Help isVisible={helpElement !== undefined}>
        <span></span>
        {helpElement}
      </Help>
    </Box>
  )
}

export default BadgeTor
