// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  Row,
  Column,
  VerticalSpace,
  HorizontalSpace
} from '../../../../components/shared/style'
import {
  LoadingSkeleton //
} from '../../../../components/shared/loading-skeleton/index'

interface Props {
  isNFT: boolean
}

export const TokenListItemSkeleton = (props: Props) => {
  const { isNFT } = props
  return (
    <Row
      padding='8px 16px'
      justifyContent='space-between'
    >
      <Row width='unset'>
        <LoadingSkeleton
          circle={true}
          width={40}
          height={40}
        />
        <HorizontalSpace space='16px' />
        <Column alignItems='flex-start'>
          <LoadingSkeleton
            width={isNFT ? 120 : 80}
            height={18}
          />
          <VerticalSpace space='2px' />
          <LoadingSkeleton
            width={120}
            height={18}
          />
        </Column>
      </Row>
      {!isNFT && (
        <Column alignItems='flex-end'>
          <LoadingSkeleton
            width={60}
            height={18}
          />
          <VerticalSpace space='2px' />
          <LoadingSkeleton
            width={40}
            height={18}
          />
        </Column>
      )}
    </Row>
  )
}

export default TokenListItemSkeleton
