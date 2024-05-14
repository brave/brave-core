// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'

// Styled Components
import {
  HoverArea,
  NameAndIcon,
  NameColumn,
  Spacer,
  BalanceColumn,
  Button
} from './style'
import { IconsWrapper } from '../../shared/style'

export const PortfolioAssetItemLoadingSkeleton = () => {
  return (
    <HoverArea noHover={true}>
      <Button disabled={true}>
        <NameAndIcon>
          <IconsWrapper>
            <LoadingSkeleton
              circle={true}
              width={32}
              height={32}
            />
          </IconsWrapper>
          <NameColumn>
            <LoadingSkeleton
              width={60}
              height={18}
            />
            <Spacer />
            <LoadingSkeleton
              width={120}
              height={18}
            />
          </NameColumn>
        </NameAndIcon>
        <BalanceColumn>
          <LoadingSkeleton
            width={60}
            height={18}
          />
          <Spacer />
          <LoadingSkeleton
            width={60}
            height={18}
          />
        </BalanceColumn>
      </Button>
    </HoverArea>
  )
}
