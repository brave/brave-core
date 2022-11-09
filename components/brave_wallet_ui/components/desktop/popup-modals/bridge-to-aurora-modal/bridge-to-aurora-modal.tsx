// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../common/locale'

import { PopupModal } from '../..'
import {
  ButtonText,
  Description,
  GlobeIcon,
  LearnMoreLink,
  OpenRainbowAppButton,
  StyledWrapper,
  Title,
  CheckboxWrapper
} from './bridge-to-aurora-modal-styles'
import { Checkbox } from '../../../shared/checkbox/checkbox'

interface Props {
  dontShowWarningAgain: boolean
  onClose: () => void
  onOpenRainbowAppClick: () => void
  onDontShowAgain: (selected: boolean) => void
}

const learnMoreLink = 'https://doc.aurora.dev/bridge/bridge-overview/'
const learnMoreRiskMitigation = 'https://rainbowbridge.app/approvals'

export const BridgeToAuroraModal = ({ dontShowWarningAgain, onClose, onOpenRainbowAppClick, onDontShowAgain }: Props) => {
  return (
    <PopupModal
      title=''
      onClose={onClose}
    >
      <StyledWrapper>
        <Title>{getLocale('braveWalletAuroraModalTitle')}</Title>
        <Description>
          {getLocale('braveWalletAuroraModalDescription')}
        </Description>
        <CheckboxWrapper>
          <Checkbox isChecked={dontShowWarningAgain} onChange={onDontShowAgain}>
            {getLocale('braveWalletAuroraModalDontShowAgain')}
          </Checkbox>
        </CheckboxWrapper>
        <OpenRainbowAppButton
          onClick={onOpenRainbowAppClick}
        >
          <GlobeIcon />
          <ButtonText>{getLocale('braveWalletAuroraModalOPenButtonText')}</ButtonText>
        </OpenRainbowAppButton>
        <LearnMoreLink
          rel='noopener noreferrer'
          target='_blank'
          href={learnMoreLink}
        >
          {getLocale('braveWalletAuroraModalLearnMore')}
        </LearnMoreLink>
        <LearnMoreLink
          rel='noopener noreferrer'
          target='_blank'
          href={learnMoreRiskMitigation}
        >
          {getLocale('braveWalletAuroraModalLearnMoreAboutRisk')}
        </LearnMoreLink>
      </StyledWrapper>
    </PopupModal>
  )
}
