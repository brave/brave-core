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
import { formatWithCommasAndDecimals } from '../../../utils/format-prices'
import { withPlaceholderIcon } from '../../shared'
import { TokenInfo } from '../../../constants/types'

export interface Props {
  action?: () => void
  assetBalance: string
  fiatBalance: string
  token: TokenInfo
}

const PortfolioAssetItem = (props: Props) => {
  const { assetBalance, fiatBalance, action, token } = props

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [])

  return (
    <>
      {token.visible &&
        <StyledWrapper onClick={action}>
          <NameAndIcon>
            <AssetIconWithPlaceholder selectedAsset={token} />
            <AssetName>{token.name}</AssetName>
          </NameAndIcon>
          <BalanceColumn>
            <FiatBalanceText>${formatWithCommasAndDecimals(fiatBalance)}</FiatBalanceText>
            <AssetBalanceText>{formatWithCommasAndDecimals(assetBalance)} {token.symbol}</AssetBalanceText>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
