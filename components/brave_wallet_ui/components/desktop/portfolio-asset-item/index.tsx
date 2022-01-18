import * as React from 'react'

// Options
import { BraveWallet, DefaultCurrencies } from '../../../constants/types'

// Utils
import { formatBalance, hexToNumber } from '../../../utils/format-balances'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'

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
import { withPlaceholderIcon } from '../../shared'

// Hooks
import { usePricing } from '../../../common/hooks'

interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  defaultCurrencies: DefaultCurrencies
}

const PortfolioAssetItem = (props: Props) => {
  const {
    spotPrices,
    assetBalance,
    action,
    token,
    defaultCurrencies
  } = props

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [])

  const formattedAssetBalance = token.isErc721
    ? formatBalance(assetBalance, token.decimals)
    : formatTokenAmountWithCommasAndDecimals(formatBalance(assetBalance, token.decimals), token.symbol)

  const { computeFiatAmount } = usePricing(spotPrices)
  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(assetBalance, token.symbol, token.decimals)
  }, [computeFiatAmount, assetBalance, token])

  return (
    <>
      {token.visible &&
        // Selecting an erc721 token is temp disabled until UI is ready for viewing NFT's
        <StyledWrapper disabled={token.isErc721} onClick={action}>
          <NameAndIcon>
            <AssetIconWithPlaceholder selectedAsset={token} />
            <AssetName>{token.name} {token.isErc721 ? hexToNumber(token.tokenId ?? '') : ''}</AssetName>
          </NameAndIcon>
          <BalanceColumn>
            {!token.isErc721 &&
              <FiatBalanceText>{formatFiatAmountWithCommasAndDecimals(fiatBalance, defaultCurrencies.fiat)}</FiatBalanceText>
            }
            <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
