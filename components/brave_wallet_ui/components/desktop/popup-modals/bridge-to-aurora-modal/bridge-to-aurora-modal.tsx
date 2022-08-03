// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../common/locale'

import { PopupModal } from '../..'
import {
  ButtonText,
  Description,
  GlobeIcon,
  LearnMoreLink,
  modalWidth,
  OpenRainbowAppButton,
  StyledWrapper,
  Title
} from './bridge-to-aurora-modal-styles'

interface Props {
  onClose: () => void
  onOpenRainbowAppClick: () => void
}

const learnMoreLink = 'https://ethereum.org/en/bridges/#bridge-risk'

export const BridgeToAuroraModal = ({ onClose, onOpenRainbowAppClick }: Props) => {
  return (
    <PopupModal
      title=''
      onClose={onClose}
      width={modalWidth}
    >
      <StyledWrapper>
        <Title>{getLocale('braveWalletAuroraModalTitle')}</Title>
        <Description>{getLocale('braveWalletAuroraModalDescription')}</Description>
        <LearnMoreLink
          target='_blank'
          href={learnMoreLink}
        >
          {getLocale('braveWalletAuroraModalLearnMore')}
        </LearnMoreLink>
        <OpenRainbowAppButton
          onClick={onOpenRainbowAppClick}
        >
          <GlobeIcon />
          <ButtonText>{getLocale('braveWalletAuroraModalOPenButtonText')}</ButtonText>
        </OpenRainbowAppButton>
      </StyledWrapper>
    </PopupModal>
  )
}
