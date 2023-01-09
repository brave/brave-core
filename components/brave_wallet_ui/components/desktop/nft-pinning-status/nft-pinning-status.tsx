// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { Row } from '../../shared/style'
import { IpfsFailureReasons } from './ipfs-failure-reasons'

import {
  StyledWrapper,
  ContentWrapper,
  UploadIcon,
  CloseIcon,
  CheckIcon,
  ReportButton,
  Text
} from './nft-pinning-status.style'

interface Props {
  pinningStatusCode: BraveWallet.TokenPinStatusCode
}

export const NftPinnigStatus = (props: Props) => {
  const { pinningStatusCode } = props
  const [icon, setIcon] = React.useState<React.ReactNode>()
  const [message, setmessage] = React.useState<string>('')
  const [reportSubmitted, setReportSubmitted] = React.useState<boolean>(false)
  const [showTooltip, setShowTooltip] = React.useState<boolean>(false)

  const onSubmitReport = React.useCallback(() => {
    setReportSubmitted(true)
  }, [])

  const onShowTooltip = React.useCallback(() => {
    if (pinningStatusCode === BraveWallet.TokenPinStatusCode.STATUS_PINNING_FAILED) {
      setShowTooltip(true)
    }
  }, [pinningStatusCode])

  React.useEffect(() => {
    switch (pinningStatusCode) {
      case BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS:
        setmessage('NFT data is being pinned to your local IPFS node')
        setIcon(<UploadIcon />)
        break

      case BraveWallet.TokenPinStatusCode.STATUS_PINNING_FAILED:
        setmessage('Cannot be pinned to your local IPFS node')
        setIcon(<CloseIcon />)
        break

      case BraveWallet.TokenPinStatusCode.STATUS_PINNED:
        setmessage('Pinned to your local IPFS node')
        setIcon(<CheckIcon />)
        break
    }
  }, [pinningStatusCode])

  return (
    <StyledWrapper>
      {showTooltip && <IpfsFailureReasons />}
      <ContentWrapper
        pinningStatus={pinningStatusCode}
        onMouseOver={onShowTooltip}
        onMouseLeave={() => setShowTooltip(false)}
      >
        {icon}
        {message}
        {pinningStatusCode === BraveWallet.TokenPinStatusCode.STATUS_PINNING_FAILED && (
          <ReportButton onClick={onSubmitReport} disabled={reportSubmitted}>
            Report issue
          </ReportButton>
        )}
      </ContentWrapper>
      {reportSubmitted && (
        <Row gap='6px'>
          <CheckIcon />
          <Text>submitted</Text>
        </Row>
      )}
    </StyledWrapper>
  )
}
