// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Styled Components
import {
  StyledWrapper
} from './style'

interface Props {
  nonFungibleTokens: BraveWallet.BlockchainToken[]
  renderToken: (item: BraveWallet.BlockchainToken, index: number) => JSX.Element | undefined | null
}

export const NFTGridView = ({
  nonFungibleTokens,
  renderToken
}: Props) => {
  return (
    <StyledWrapper>
      {nonFungibleTokens.map(renderToken)}
    </StyledWrapper>
  )
}
