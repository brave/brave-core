// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { WalletSelectors } from '../../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'
import Amount from '../../../../../utils/amount'
import { computeFiatAmount } from '../../../../../utils/pricing-utils'
import { formatTokenBalanceWithSymbol } from '../../../../../utils/balance-utils'
import { checkIfTokenNeedsNetworkIcon } from '../../../../../utils/asset-utils'
import { useGetNetworkQuery } from '../../../../../common/slices/api.slice'

// Components
import {
  withPlaceholderIcon,
  CreateNetworkIcon
} from '../../../../../components/shared'
import { NftIcon } from '../../../../../components/shared/nft-icon/nft-icon'
import { NFTInfoTooltip } from '../nft-info-tooltip/nft-info-tooltip'

// Styled Components
import {
  AssetIcon,
  NetworkIconWrapper,
  Button,
  IconsWrapper,
  ButtonWrapper,
  IconAndName
} from './token-list-item.style'
import { Column, Text } from '../../shared.styles'

interface Props {
  onClick: () => void
  token: BraveWallet.BlockchainToken
  balance: string
}

export const TokenListItem = (props: Props) => {
  const { onClick, token, balance } = props

  // Wallet Selectors
  const spotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // Queries
  const { data: tokensNetwork } = useGetNetworkQuery(token, { skip: !token })

  // Memos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(token?.isErc721 ? NftIcon : AssetIcon, {
      size: 'big',
      marginLeft: 0,
      marginRight: 0
    })
  }, [token?.isErc721])

  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(spotPrices, {
      decimals: token.decimals,
      symbol: token.symbol,
      value: balance,
      contractAddress: token.contractAddress,
      chainId: token.chainId
    })
  }, [
    spotPrices,
    balance,
    token.symbol,
    token.decimals,
    token.contractAddress,
    token.chainId
  ])

  const formattedFiatBalance = React.useMemo(() => {
    return fiatBalance.formatAsFiat(defaultCurrencies.fiat)
  }, [fiatBalance, defaultCurrencies.fiat])

  const networkDescription = React.useMemo(() => {
    if (tokensNetwork) {
      return token.symbol !== ''
        ? getLocale('braveWalletPortfolioAssetNetworkDescription')
          .replace('$1', token.symbol)
          .replace('$2', tokensNetwork.chainName ?? '')
        : tokensNetwork.chainName
    }
    return token.symbol
  }, [tokensNetwork, token.symbol])

  const tokenDisplayName = React.useMemo(() => {
    if (token.isErc20 || token.isNft) {
      const id = token.tokenId ? `#${new Amount(token.tokenId).toNumber()}` : ''
      return `${token.name} ${id}`
    }
    return token.name
  }, [token.name, token.isErc20, token.isNft])

  return (
    <ButtonWrapper>
      <Button onClick={onClick}>
        <IconAndName horizontalAlign='flex-start'>
          <IconsWrapper>
            <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
            {
              tokensNetwork &&
              checkIfTokenNeedsNetworkIcon(tokensNetwork, token.contractAddress) &&
              <NetworkIconWrapper>
                <CreateNetworkIcon network={tokensNetwork} marginRight={0} />
              </NetworkIconWrapper>
            }
          </IconsWrapper>
          <Column horizontalAlign='flex-start'>
            <Text textColor='text01' textSize='14px' isBold={true} textAlign='left'>
              {tokenDisplayName}
            </Text>
            <Text textColor='text03' textSize='12px' isBold={false} textAlign='left'>
              {networkDescription}
            </Text>
          </Column>
        </IconAndName>
        {!token.isErc721 && !token.isNft &&
          <Column horizontalAlign='flex-end'>
            <Text textColor='text01' textSize='14px' isBold={true}>
              {formatTokenBalanceWithSymbol(balance, token.decimals, token.symbol)}
            </Text>
            <Text textColor='text03' textSize='12px' isBold={false}>
              {formattedFiatBalance}
            </Text>
          </Column>
        }
      </Button>
      {(token.isErc721 || token.isNft) &&
        <NFTInfoTooltip network={tokensNetwork} token={token} />
      }
    </ButtonWrapper>
  )
}

export default TokenListItem
