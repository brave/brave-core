// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { checkIfTokenNeedsNetworkIcon } from '../../../utils/asset-utils'

// components
import { IconsWrapper, MediumAssetIcon, NetworkIconWrapper } from '../style'
import { withPlaceholderIcon, CreateNetworkIcon } from '..'

// styles
import {
  BuyAssetOptionWrapper,
  AssetName,
  NameAndIcon,
  NameColumn,
  NetworkDescriptionText,
  PriceContainer,
  PriceText
} from './buy-asset-option.styles'
import { useApiProxy } from '../../../common/hooks/use-api-proxy'
import { getTokenParam } from '../../../utils/api-utils'
import { LoadIcon } from './buy-option-item-styles'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

interface Props {
  onClick?: (token: BraveWallet.BlockchainToken) => void
  token: BraveWallet.BlockchainToken
  isSelected?: boolean
  isPanel?: boolean
  /** Set this to a currency-code to fetch & display the token's price */
  selectedCurrency?: string
}

const AssetIconWithPlaceholder = withPlaceholderIcon(MediumAssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })

export const BuyAssetOptionItem = React.forwardRef<HTMLButtonElement, Props>(({
  onClick,
  token,
  isSelected,
  isPanel,
  selectedCurrency
}: Props, forwardedRef) => {
  // state
  const [price, setPrice] = React.useState('')
  const [isFetchingPrice, setIsFetchingPrice] = React.useState(!!selectedCurrency)

  // queries
  const { data: tokenNetwork } = useGetNetworkQuery(token, { skip: !token })

  // custom hooks
  const { assetRatioService } = useApiProxy()

  // memos
  const networkDescription: string = React.useMemo(() => {
    if (tokenNetwork && !isPanel) {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokenNetwork.chainName ?? '')
    }
    return token.symbol
  }, [tokenNetwork, isPanel, token])

  // methods
  const handleOnClick = React.useCallback(() => {
    if (onClick) {
      onClick(token)
    }
  }, [onClick, token])

  // effects
  React.useEffect(() => {
    // fetch asset price

    let subscribed = true

    // need a selected currency to show price
    if (selectedCurrency) {
      const tokenParam = getTokenParam(token)
      setIsFetchingPrice(true)
      assetRatioService.getPrice(
        [tokenParam],
        [selectedCurrency.toLowerCase()],
        1 as BraveWallet.AssetPriceTimeframe // one day
      ).then(({ values, success }) => {
        if (!subscribed) {
          return
        }
        setIsFetchingPrice(false)
        setPrice(values?.[0]?.price || '')
      })
    }

    // cleanup
    return () => {
      subscribed = false
    }
  }, [selectedCurrency, assetRatioService])

  // render
  if (!token.visible) {
    return null
  }

  return (
    <BuyAssetOptionWrapper ref={forwardedRef} isSelected={isSelected} onClick={handleOnClick}>
      <NameAndIcon>
        <IconsWrapper marginRight='14px'>
          <AssetIconWithPlaceholder asset={token} network={tokenNetwork} />
          {
            tokenNetwork &&
            !isPanel && checkIfTokenNeedsNetworkIcon(tokenNetwork, token.contractAddress) &&
            <NetworkIconWrapper>
              <CreateNetworkIcon network={tokenNetwork} marginRight={0} />
            </NetworkIconWrapper>
          }
        </IconsWrapper>
        <NameColumn>
          <AssetName>
            {token.name} {
              token.isErc721 && token.tokenId
                ? '#' + new Amount(token.tokenId).toNumber()
                : ''
            }
          </AssetName>
          <NetworkDescriptionText>{networkDescription}</NetworkDescriptionText>
        </NameColumn>
      </NameAndIcon>

      {selectedCurrency &&
          <PriceContainer>
            {isFetchingPrice
              ? <LoadIcon />
              : !!price && <PriceText>
                  {new Amount(price).formatAsFiat(selectedCurrency)}
                </PriceText>
            }
          </PriceContainer>
        }
    </BuyAssetOptionWrapper>
  )
}
)

export default BuyAssetOptionItem
