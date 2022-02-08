import * as React from 'react'

// Options
import { BraveWallet, DefaultCurrencies } from '../../../constants/types'

// Utils
import { formatBalance, hexToNumber } from '../../../utils/format-balances'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AssetName,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AssetIcon,
  IconsWrapper,
  NetworkIconWrapper,
  NameColumn
} from './style'
import { withPlaceholderIcon, CreateNetworkIcon } from '../../shared'
import { WithHideBalancePlaceholder } from '../'

// Hooks
import { usePricing } from '../../../common/hooks'

interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  defaultCurrencies: DefaultCurrencies
  hideBalances?: boolean
  selectedNetwork?: BraveWallet.EthereumChain
}

const PortfolioAssetItem = (props: Props) => {
  const {
    spotPrices,
    assetBalance,
    action,
    token,
    defaultCurrencies,
    hideBalances,
    selectedNetwork
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

  const NetworkDescription = React.useMemo(() => {
    if (selectedNetwork && token.contractAddress !== '') {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', selectedNetwork?.chainName ?? '')
    }
    return token.symbol
  }, [selectedNetwork, token])

  return (
    <>
      {token.visible &&
        // Selecting an erc721 token is temp disabled until UI is ready for viewing NFT's
        <StyledWrapper disabled={token.isErc721} onClick={action}>
          <NameAndIcon>
            <IconsWrapper>
              <AssetIconWithPlaceholder selectedAsset={token} />
              {selectedNetwork && token.contractAddress !== '' &&
                <NetworkIconWrapper>
                  <CreateNetworkIcon network={selectedNetwork} marginRight={0} />
                </NetworkIconWrapper>
              }
            </IconsWrapper>
            <NameColumn>
              <AssetName>{token.name} {token.isErc721 ? hexToNumber(token.tokenId ?? '') : ''}</AssetName>
              <AssetName>{NetworkDescription}</AssetName>
            </NameColumn>
          </NameAndIcon>
          <BalanceColumn>
            <WithHideBalancePlaceholder
              size='small'
              hideBalances={hideBalances ?? false}
            >
              {!token.isErc721 &&
                <FiatBalanceText>{formatFiatAmountWithCommasAndDecimals(fiatBalance, defaultCurrencies.fiat)}</FiatBalanceText>
              }
              <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
            </WithHideBalancePlaceholder>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
