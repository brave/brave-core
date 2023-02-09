// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { LoadingSkeleton } from '../../shared'

// Styled Components
import { StyledWrapper, NameAndIcon, NameColumn, Spacer, BalanceColumn, ButtonArea } from './style'
import { IconsWrapper } from '../../shared/style'

export const PortfolioAssetItemLoadingSkeleton = () => {
  return (
    <StyledWrapper>
      <ButtonArea disabled={true}>
      <NameAndIcon>
        <IconsWrapper>
          <LoadingSkeleton
            circle={true}
            width={40}
            height={40}
          />
        </IconsWrapper>
        <NameColumn>
          <LoadingSkeleton width={60} height={18} />
          <Spacer />
          <LoadingSkeleton width={120} height={18} />
        </NameColumn>
      </NameAndIcon>
      <BalanceColumn>
        <LoadingSkeleton width={60} height={18} />
        <Spacer />
        <LoadingSkeleton width={60} height={18} />
      </BalanceColumn>
      </ButtonArea>
    </StyledWrapper>
  )
}
