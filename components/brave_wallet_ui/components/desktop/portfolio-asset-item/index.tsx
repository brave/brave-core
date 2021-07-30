import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AssetName,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AssetIcon
} from './style'

export interface Props {
  action?: () => void
  name: string
  symbol: string
  icon: string
  assetBalance: number
  fiatBalance: string
}

const PortfolioAssetItem = (props: Props) => {
  const { name, assetBalance, fiatBalance, icon, symbol, action } = props
  return (
    <StyledWrapper onClick={action}>
      <NameAndIcon>
        <AssetIcon icon={icon} />
        <AssetName>{name}</AssetName>
      </NameAndIcon>
      <BalanceColumn>
        <FiatBalanceText>${fiatBalance}</FiatBalanceText>
        <AssetBalanceText>{assetBalance.toFixed(4)} {symbol}</AssetBalanceText>
      </BalanceColumn>
    </StyledWrapper>
  )
}

export default PortfolioAssetItem
