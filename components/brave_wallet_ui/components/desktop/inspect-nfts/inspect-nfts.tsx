// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// components
import { NftList } from './components/nft-list/nft-list'
import Illustration from '../../../assets/svg-icons/nft-ipfs/pinned-nft-illustration.svg'
import { InfoTooltip } from './components/info-tooltip/info-tooltip'
import { IpfsNodeStatus } from '../views/portfolio/components/ipfs-node-status/ipfs-node-status'

// selectors
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'

// utils
import { WalletPageActions } from '../../../page/actions'

// styles
import {
  InspectNftsWrapper,
  TopRow,
  TopRowButton,
  CloseIcon,
  BackIcon,
  MainContent,
  PinNftsButton,
  InfoSubHeading,
  InfoIcon,
  SubDivider,
  PinnedNftIllustration
} from './inspects-nfts.styles'

import {
  ActionButton,
  BenefitHeading,
  BenefitsList
} from '../local-ipfs-node/local-ipfs-node.styles'
import { Column, Row } from '../../shared/style'
import { WalletRoutes } from '../../../constants/types'

interface Props {
  onClose: () => void
  onBack: () => void
}

export const InspectNftsScreen = ({ onClose }: Props) => {
  const [showTooltip, setShowTooltip] = React.useState<boolean>(false)

  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const isAutoPinEnabled = useSafePageSelector(PageSelectors.isAutoPinEnabled)

  const onClickRunNode = React.useCallback(() => {
    dispatch(WalletPageActions.setAutoPinEnabled(true))
  }, [])

  const onShowTooltip = React.useCallback(() => setShowTooltip(true), [])
  const onHideTooltip = React.useCallback(() => setShowTooltip(false), [])

  const goToNftsTab = React.useCallback(() => {
    history.push(WalletRoutes.Nfts)
  }, [])

  return (
    <InspectNftsWrapper>
      <TopRow>
        <TopRowButton onClick={onClose}>
          <BackIcon />
          Back
        </TopRowButton>
        <TopRowButton onClick={onClose}>
          Close
          <CloseIcon />
        </TopRowButton>
      </TopRow>
      <MainContent>
        <Row maxWidth='100%' alignItems='center' justifyContent='center'>
          <NftList />
        </Row>
        <Row
          margin='35px 0 11px 0'
          gap='8px'
          alignItems='center'
          justifyContent='flex-end'
          onMouseEnter={onShowTooltip}
          onMouseLeave={onHideTooltip}
        >
          <InfoSubHeading>Why not available</InfoSubHeading>
          <InfoIcon/>
          {showTooltip && (
            <InfoTooltip text='Some of NFT data are stored in centralized server such as AWS, Google Cloud, etc. In this case, it is not available to pin your NFT data to IPFS network.' />
          )}
        </Row>
        <Row>
          <SubDivider />
        </Row>
        <Column margin='32px 0 0'>
          <BenefitHeading>
            By running IPFS on your computer you can:
          </BenefitHeading>
          <BenefitsList>
            <li>
              Ensure your NFT data stays online and it cannot be tempered with.
            </li>
            <li>
              Get content back using content identifier(CID) from an incorrect
              data.
            </li>
            <li>
              Participate proof of authenticity and make IPFS network rich and
              healthy.
            </li>
          </BenefitsList>
        </Column>
        <Row gap='16px' alignItems='center' justifyContent='flex-start'>
          {isAutoPinEnabled
            ? <>
              <PinNftsButton onClick={goToNftsTab}>Keep my NFTs always online</PinNftsButton>
              <IpfsNodeStatus />
            </>
            : <ActionButton onClick={onClickRunNode}>Run my local IPFS Node</ActionButton>
          }
        </Row>
        <PinnedNftIllustration src={Illustration} />
      </MainContent>
    </InspectNftsWrapper>
  )
}
