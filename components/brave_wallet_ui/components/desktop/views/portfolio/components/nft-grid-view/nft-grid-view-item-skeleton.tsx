// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { LoadingSkeleton } from '../../../../../shared/loading-skeleton/index'
import { VerticalSpace } from '../../../../../shared/style'
import { NFTWrapper } from './style'

export const NftGridViewItemSkeleton = () => {
  return (
    <NFTWrapper>
      <LoadingSkeleton height={222} />
      <VerticalSpace space='8px' />
      <LoadingSkeleton
        width='80%'
        height={20}
      />
      <VerticalSpace space='8px' />
      <LoadingSkeleton
        width={40}
        height={20}
      />
    </NFTWrapper>
  )
}
