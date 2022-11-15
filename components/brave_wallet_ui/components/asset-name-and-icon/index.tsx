// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { StyledWrapper, NameAndSymbolWrapper, AssetName, AssetSymbol, AssetIcon } from './style'

export interface Props {
  symbol: string
  assetName: string
  assetLogo: string
}

export const AssetNameAndIcon = (props: Props) => {
  const {
    assetLogo,
    assetName,
    symbol
  } = props

  return (
    <StyledWrapper>
      <AssetIcon src={assetLogo} loading="lazy" />
      <NameAndSymbolWrapper>
        <AssetName>{assetName}</AssetName>
        <AssetSymbol>{symbol}</AssetSymbol>
      </NameAndSymbolWrapper>
    </StyledWrapper>
  )
}
