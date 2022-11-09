// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// components
import { NavButton } from '../buttons/nav-button/index'

// style
import {
  VerticalSpace,
  WalletWelcomeGraphic
} from '../../shared/style'
import {
  StyledWrapper,
  Title,
  Description
} from './style'

interface Props {
  onSetup: () => void
}

export const WelcomePanel = ({ onSetup }: Props) => {
  return (
    <StyledWrapper>
      <WalletWelcomeGraphic scale={0.9}/>

      <VerticalSpace space='16px' />

      <Title>{getLocale('braveWalletPanelTitle')}</Title>
      <Description>
        {getLocale('braveWalletWelcomePanelDescription')}
      </Description>

      <NavButton
        buttonType='primary'
        text={getLocale('braveWalletLearnMore')}
        onSubmit={onSetup}
      />

      <VerticalSpace space='16px' />
    </StyledWrapper>
  )
}

export default WelcomePanel
