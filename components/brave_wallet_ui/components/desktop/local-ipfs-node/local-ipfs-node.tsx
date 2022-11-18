// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { WalletRoutes } from '../../../constants/types'
import { Row } from '../../shared/style'

import Illustration from '../../../assets/svg-icons/nft-ipfs/nfts-illustration.svg'

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
  RunLocalNodeButton,
  CheckNftsButton,
  BenefitHeading,
  LeftWrapper,
  RightWrapper,
  BenefitsList,
  IpfsNodeRunningStatus,
  IpfsStatus,
  NftIllustration
} from './local-ipfs-node.styles'

interface Props {
  onClose: () => void
}

export const LocalIpfsNodeScreen = (props: Props) => {
  const { onClose } = props
  const history = useHistory()

  const onClickCheckNfts = React.useCallback(() => {
    history.push(WalletRoutes.InspectNfts)
  }, [])

  return (
    <RunNodeWrapper>
      <TopRow>
        <CloseButton onClick={onClose}>
          Close
          <CloseIcon />
        </CloseButton>
      </TopRow>
      <MainContent>
        <HeadingWrapper>
          <IpfsIcon />
          <Heading>A big step toward becoming part of web3</Heading>
        </HeadingWrapper>
        <Section>
          <LeftWrapper>
            <SectionText>
              Run IPFS node and keep your NFTs always online
            </SectionText>
          </LeftWrapper>
          <RightWrapper>
            <Description>
              Today's centralized internet model doesn't work in space. On today’s
              internet, every time you click something, that data has to be
              retrieved from a centralized server. If you are on the moon, there
              will be a delay with every click, as content is retrieved from
              earth. Using IPFS, content is retrieved from wherever is closest. If
              someone else nearby on the moon has already retrieved that data,
              your data can be retrieved by you or them on the moon. So is your
              NFT data.
            </Description>
          </RightWrapper>
        </Section>
        <Section>
          <LeftWrapper>
            <Row gap='16px' alignItems='center' justifyContent='flex-start'>
              <RunLocalNodeButton>Run my local IPFS Node</RunLocalNodeButton>
              <IpfsNodeRunningStatus>
                <IpfsStatus />
                You’re running IPFS node
              </IpfsNodeRunningStatus>
            </Row>
            <CheckNftsButton onClick={onClickCheckNfts}>Check which NFTs of mine can be pinned</CheckNftsButton>
          </LeftWrapper>
          <RightWrapper>
            <BenefitHeading>By running IPFS on your computer you can:</BenefitHeading>
            <BenefitsList>
              <li>Ensure your NFT data stays online and it cannot be tempered with.</li>
              <li>Get content back using content identifier(CID) from an incorrect data.</li>
              <li>Participate proof of authenticity and make IPFS network rich and healthy.</li>
            </BenefitsList>
          </RightWrapper>
        </Section>
      </MainContent>
      <NftIllustration src={Illustration} />
    </RunNodeWrapper>
  )
}
