import * as React from 'react'

// Options
import { ETH } from '../../../options/asset-options'

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
import { formatWithCommasAndDecimals } from '../../../utils/format-prices'
export interface Props {
  action?: () => void
  name: string
  symbol: string
  logo?: string
  assetBalance: string
  fiatBalance: string
  isVisible?: boolean
}

const PortfolioAssetItem = (props: Props) => {
  const { name, assetBalance, fiatBalance, logo, symbol, isVisible, action } = props
  return (
    <>
      {isVisible &&
        <StyledWrapper onClick={action}>
          <NameAndIcon>
            <AssetIcon icon={(symbol === 'ETH' ? ETH.asset.logo : logo) ?? ''} />
            <AssetName>{name}</AssetName>
          </NameAndIcon>
          <BalanceColumn>
            <FiatBalanceText>${formatWithCommasAndDecimals(fiatBalance)}</FiatBalanceText>
            <AssetBalanceText>{formatWithCommasAndDecimals(assetBalance)} {symbol}</AssetBalanceText>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
