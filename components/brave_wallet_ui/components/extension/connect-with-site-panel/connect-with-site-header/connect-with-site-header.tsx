// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Types
import { SerializableOriginInfo } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { CreateSiteOrigin, Tooltip } from '../../../shared'

// Styled Components
import {
  StyledWrapper,
  FavIcon,
  Title,
  TitleWrapper,
  SiteName,
  SiteURL,
  MessageBox,
  InfoIcon,
  BackButton,
  BackIcon,
  GradientLine,
  LinkIconCircle,
  LinkIcon
} from './connect-with-site-header.style'
import { AccountCircle } from '../select-account-item/select-account-item.style'
import { HorizontalSpace, Column, Row } from '../../../shared/style'

interface Props {
  originInfo: SerializableOriginInfo
  isReadyToConnect: boolean
  onBack: () => void
  address?: string
  isScrolled: boolean
}

export const ConnectWithSiteHeader = (props: Props) => {
  const { originInfo, address, isReadyToConnect, isScrolled, onBack } = props

  // Memos
  const orb = React.useMemo(() => {
    return create({
      seed: address?.toLowerCase() ?? '',
      size: 8,
      scale: 16
    }).toDataURL()
  }, [address])

  return (
    <>
      <TitleWrapper
        isScrolled={isScrolled}
        isReadyToConnect={isReadyToConnect}
        padding="12px 16px"
        justifyContent="space-between"
      >
        {isReadyToConnect ? (
          <BackButton onClick={onBack}>
            <BackIcon name="arrow-left" />
          </BackButton>
        ) : (
          <HorizontalSpace space="24px" />
        )}
        <Title>{getLocale('braveWalletConnectWallet')}</Title>
        <HorizontalSpace space="24px" />
      </TitleWrapper>

      <StyledWrapper isScrolled={isScrolled}>
        <Column
          padding={`${isReadyToConnect ? '30px' : '24px'} 16px 24px 16px`}
          fullWidth={true}
          alignItems="flex-start"
        >
          {isReadyToConnect && (
            <Row justifyContent="center" marginBottom={18}>
              <Tooltip
                isAddress={true}
                minWidth={120}
                maxWidth={120}
                text={address}
              >
                <AccountCircle orb={orb} />
              </Tooltip>
              <GradientLine>
                <LinkIconCircle>
                  <LinkIcon name="link-normal" />
                </LinkIconCircle>
              </GradientLine>
              <Tooltip
                text={originInfo.eTldPlusOne}
              >
                <FavIcon
                  src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`}
                  isReadyToConnect={isReadyToConnect}
                />
              </Tooltip>
            </Row>
          )}

          {!isReadyToConnect && (
            <Row justifyContent="flex-start" marginBottom={16}>
              <FavIcon
                src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`}
                isReadyToConnect={isReadyToConnect}
              />
              <Column alignItems="flex-start">
                <SiteName>{originInfo.eTldPlusOne}</SiteName>
                <SiteURL>
                  <CreateSiteOrigin
                    originSpec={originInfo.originSpec}
                    eTldPlusOne={originInfo.eTldPlusOne}
                  />
                </SiteURL>
              </Column>
            </Row>
          )}

          <MessageBox padding="8px 16px" justifyContent="flex-start">
            <InfoIcon name="info-filled" />
            {getLocale('braveWalletConnectTrustWarning')}
          </MessageBox>
        </Column>
      </StyledWrapper>
    </>
  )
}

export default ConnectWithSiteHeader
