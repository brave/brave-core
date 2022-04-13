import * as React from 'react'

// Options
import { BraveWallet, DefaultCurrencies } from '../../../constants/types'

// Utils
import Amount from '../../../utils/amount'
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
  NameColumn,
  Spacer,
  NetworkDescriptionText
} from './style'
import { withPlaceholderIcon, CreateNetworkIcon, LoadingSkeleton } from '../../shared'
import { WithHideBalancePlaceholder } from '../'

import { getTokensNetwork } from '../../../utils/network-utils'

// Hooks
import { usePricing } from '../../../common/hooks'
import { unbiasedRandom } from '../../../utils/random-utils'

interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  defaultCurrencies: DefaultCurrencies
  hideBalances?: boolean
  networks: BraveWallet.NetworkInfo[]
  isPanel?: boolean
}

const PortfolioAssetItem = (props: Props) => {
  const {
    spotPrices,
    assetBalance,
    action,
    token,
    defaultCurrencies,
    hideBalances,
    isPanel,
    networks
  } = props
  const [assetNameSkeletonWidth, setAssetNameSkeletonWidth] = React.useState(0)
  const [assetNetworkSkeletonWidth, setAssetNetworkSkeletonWidth] = React.useState(0)

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [])

  const formattedAssetBalance = token.isErc721
    ? new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .format()
    : new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .formatAsAsset(6, token.symbol)

  const { computeFiatAmount } = usePricing(spotPrices)
  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(assetBalance, token.symbol, token.decimals)
  }, [computeFiatAmount, assetBalance, token])

  const formattedFiatBalance = React.useMemo(() => {
    return fiatBalance.formatAsFiat(defaultCurrencies.fiat)
  }, [fiatBalance])

  const isLoading = React.useMemo(() => {
    return formattedAssetBalance === '' && !token.isErc721
  }, [formattedAssetBalance, token])

  const tokensNetwork = React.useMemo(() => {
    return getTokensNetwork(networks, token)
  }, [token, networks])

  const NetworkDescription = React.useMemo(() => {
    if (tokensNetwork && !isPanel) {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokensNetwork.chainName ?? '')
    }
    return token.symbol
  }, [tokensNetwork, token])

  React.useEffect(() => {
    // Randow value between 100 & 250
    // Set value only once
    if (assetNameSkeletonWidth === 0) {
      setAssetNameSkeletonWidth(unbiasedRandom(100, 250))
    }

    if (assetNetworkSkeletonWidth === 0) {
      setAssetNetworkSkeletonWidth(unbiasedRandom(100, 250))
    }
  }, [])

  return (
    <>
      {token.visible &&
        // Selecting an erc721 token is temp disabled until UI is ready for viewing NFTs
        // or when showing loading skeleton
        <StyledWrapper disabled={isLoading} onClick={action}>
          <NameAndIcon>
            <IconsWrapper>
              {isLoading
                ? <LoadingSkeleton
                  circle={true}
                  width={40}
                  height={40}
                />
                : <>
                  <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
                  {tokensNetwork && token.contractAddress !== '' && !isPanel &&
                    <NetworkIconWrapper>
                      <CreateNetworkIcon network={tokensNetwork} marginRight={0} />
                    </NetworkIconWrapper>
                  }
                </>
              }
            </IconsWrapper>
            <NameColumn>
              {isLoading
                ? <>
                  <LoadingSkeleton width={assetNameSkeletonWidth} height={18} />
                  <Spacer />
                  <LoadingSkeleton width={assetNetworkSkeletonWidth} height={18} />
                </>
                : <>
                  <AssetName>
                    {token.name} {
                      token.isErc721 && token.tokenId
                        ? '#' + new Amount(token.tokenId).toNumber()
                        : ''
                    }
                  </AssetName>
                  <NetworkDescriptionText>{NetworkDescription}</NetworkDescriptionText>
                </>
              }
            </NameColumn>
          </NameAndIcon>
          <BalanceColumn>
            <WithHideBalancePlaceholder
              size='small'
              hideBalances={hideBalances ?? false}
            >
              {isLoading
                ? <>
                  <LoadingSkeleton width={100} height={20} />
                </>
                : <>
                  {!token.isErc721 &&
                    <FiatBalanceText>{formattedFiatBalance}</FiatBalanceText>
                  }
                  <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
                </>
              }
            </WithHideBalancePlaceholder>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
