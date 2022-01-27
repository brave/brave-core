import * as React from 'react'

import { StyledWrapper, NameAndSymbolWrapper, AssetName, AssetSymbol, AssetIcon } from './style'

export interface Props {
  symbol: string
  assetName: string
  assetLogo: string
}

const AssetNameAndIcon = (props: Props) => {
  const { assetLogo, assetName, symbol } = props

  return (
    <StyledWrapper>
      <AssetIcon src={`chrome://image?${assetLogo}`} loading="lazy" />
      <NameAndSymbolWrapper>
        <AssetName>{assetName}</AssetName>
        <AssetSymbol>{symbol}</AssetSymbol>
      </NameAndSymbolWrapper>
    </StyledWrapper>
  )
}

export default AssetNameAndIcon
