// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { Row } from '../../shared/style'
import Illustration from '../../../assets/png-icons/nft-ipfs/nfts-illustration.png'

// styles
import {
  CloseButton,
  CloseIcon,
  HeadingWrapper,
  Heading,
  RunNodeWrapper,
  TopRow,
  MainContent,
  IpfsIcon,
  Section,
  SectionText,
  Description,
  LeftWrapper,
  RightWrapper,
  NftIllustration
} from './local-ipfs-node.styles'
import { getLocale, splitStringForTag } from '../../../../common/locale'

interface Props {
  onClose: () => void
}

export const LocalIpfsNodeScreen = (props: Props) => {
  const { onClose } = props


  // redux
  const { beforeTag, afterTag } = splitStringForTag(
    getLocale('braveWalletNftPinningRunNodeDescription')
  )


  return (
    <RunNodeWrapper>
      <TopRow>
        <CloseButton onClick={onClose}>
          {getLocale('braveWalletNftPinningCloseButton')}
          <CloseIcon />
        </CloseButton>
      </TopRow>
      <MainContent>
        <HeadingWrapper>
          <IpfsIcon />
          <Heading>{getLocale('braveWalletNftPinningHeading')}</Heading>
        </HeadingWrapper>
        <Section>
          <LeftWrapper>
            <SectionText>
              {getLocale('braveWalletNftPinningRunNodeHeading')}
            </SectionText>
          </LeftWrapper>
          <RightWrapper>
            <Description>
              {beforeTag}
              <br />
              <br />
              {afterTag}
            </Description>
          </RightWrapper>
        </Section>
        <Section>
          <LeftWrapper>
            <Row
              gap='16px'
              alignItems='center'
              justifyContent='flex-start'
              margin='0 0 0 8px'
            >
            </Row>
          </LeftWrapper>
        </Section>
      </MainContent>
      <NftIllustration src={Illustration} />
    </RunNodeWrapper>
  )
}
