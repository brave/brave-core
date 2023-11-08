// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'

// Styled Components
import { CreatingWalletText, LoadingIcon } from './creating_wallet.style'
import { StyledWrapper, MainWrapper } from '../onboarding.style'
import { Column } from '../../../../components/shared/style'

export const CreatingWallet = () => {
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>
          <Column
            fullWidth={true}
            padding='200px 0px'
          >
            <LoadingIcon />
            <CreatingWalletText>
              {getLocale('braveWalletCreatingWallet')}
            </CreatingWalletText>
          </Column>
        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}
