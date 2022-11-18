// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
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

export type PinningStatus = 'uploading' | 'pinned' | 'failed' | 'centralized'

interface Props {
  pinningStatus: PinningStatus
}

export const NftPinnigStatus = (props: Props) => {
  const { pinningStatus } = props
  const [icon, setIcon] = React.useState<React.ReactNode>()
  const [message, setmessage] = React.useState<string>('')
  const [reportSubmitted, setReportSubmitted] = React.useState<boolean>(false)
  const [showTooltip, setShowTooltip] = React.useState<boolean>(false)

  const onSubmitReport = React.useCallback(() => {
    setReportSubmitted(true)
  }, [])

  const onShowTooltip = React.useCallback(() => {
    if (pinningStatus === 'failed') {
      setShowTooltip(true)
    }
  }, [pinningStatus])

  React.useEffect(() => {
    switch (pinningStatus) {
      case 'uploading':
        setmessage('NFT data is being pinned to your local IPFS node')
        setIcon(<UploadIcon />)
        break

      case 'centralized':
        setmessage('Cannot be pinned to your local IPFS node')
        setIcon(<CloseIcon />)
        break

      case 'failed':
        setmessage('Failed pinning. We will retry.')
        setIcon(<CloseIcon />)
        break

      case 'pinned':
        setmessage('Pinned to your local IPFS node')
        setIcon(<CheckIcon />)
        break
    }
  }, [pinningStatus])

  return (
    <StyledWrapper pinningStatus={pinningStatus}>
      {showTooltip && <IpfsFailureReasons />}
      <ContentWrapper
        pinningStatus={pinningStatus}
        onMouseOver={onShowTooltip}
        onMouseLeave={() => setShowTooltip(false)}
      >
        {icon}
        {message}
        {pinningStatus === 'failed' && (
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
