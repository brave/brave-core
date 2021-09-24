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
import { formatPrices } from '../../../utils/format-prices'
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
            <AssetIcon icon={logo ? logo : ''} />
            <AssetName>{name}</AssetName>
          </NameAndIcon>
          <BalanceColumn>
            <FiatBalanceText>${formatPrices(Number(fiatBalance))}</FiatBalanceText>
            <AssetBalanceText>{Number(assetBalance).toFixed(4)} {symbol}</AssetBalanceText>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
