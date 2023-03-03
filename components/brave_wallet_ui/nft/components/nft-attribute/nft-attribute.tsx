// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { NFTAtrribute } from '../../../constants/types'

import { StyledWrapper, TraitType, TraitValue } from './nft-attribute.styles'

interface Props {
  attribute: NFTAtrribute
}

export const NftAttribute = ({ attribute }: Props) => {
  const { traitType, value } = attribute

  return (
    <StyledWrapper>
      <TraitType>{traitType}</TraitType>
      <TraitValue>{value}</TraitValue>
    </StyledWrapper>
  )
}