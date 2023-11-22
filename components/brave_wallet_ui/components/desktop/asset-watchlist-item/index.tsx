// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useLocation } from 'react-router-dom'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

// Components
import {
  withPlaceholderIcon //
} from '../../shared/create-placeholder-icon/index'
import { NftIcon } from '../../shared/nft-icon/nft-icon'

// Styled Components
import {
  StyledWrapper,
  AssetName,
  NameAndIcon,
  AssetIcon,
  Button,
  Icon,
  RightSide,
  NameAndSymbol,
  AssetSymbol
} from './style'
import { HorizontalSpace } from '../../shared/style'

export interface Props {
  onSelectAsset: (selected: boolean, token: BraveWallet.BlockchainToken) => void
  onRemoveAsset: (token: BraveWallet.BlockchainToken) => void
  isRemovable: boolean
  isSelected: boolean
  token: BraveWallet.BlockchainToken
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 8 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

const AssetWatchlistItem = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const { onSelectAsset, onRemoveAsset, isRemovable, token, isSelected } =
      props

    // routing
    const { hash } = useLocation()

    // queries
    const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)

    // callbacks
    const onCheck = React.useCallback(() => {
      onSelectAsset(!isSelected, token)
    }, [onSelectAsset, token, isSelected])

    const onClickAsset = React.useCallback(() => {
      onSelectAsset(!isSelected, token)
    }, [onSelectAsset, token, isSelected])

    const onClickRemoveAsset = React.useCallback(() => {
      onRemoveAsset(token)
    }, [token, onRemoveAsset])

    const networkDescription = React.useMemo(() => {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokensNetwork?.chainName ?? '')
    }, [tokensNetwork, token])

    return (
      <StyledWrapper ref={forwardedRef}>
        <NameAndIcon onClick={onClickAsset}>
          {token.isNft ? (
            <NftIconWithPlaceholder
              asset={token}
              network={tokensNetwork}
            />
          ) : (
            <AssetIconWithPlaceholder
              asset={token}
              network={tokensNetwork}
            />
          )}
          <NameAndSymbol>
            <AssetName>
              {token.name}{' '}
              {token.isErc721 && token.tokenId
                ? '#' + new Amount(token.tokenId).toNumber()
                : ''}
            </AssetName>
            <AssetSymbol>{networkDescription}</AssetSymbol>
          </NameAndSymbol>
        </NameAndIcon>
        <RightSide>
          {isRemovable && hash !== WalletRoutes.AvailableAssetsHash && (
            <>
              <Button onClick={onClickRemoveAsset}>
                <Icon name='trash' />
              </Button>
              <HorizontalSpace space='8px' />
            </>
          )}
          <Button onClick={onCheck}>
            <Icon
              name={
                hash === WalletRoutes.AvailableAssetsHash
                  ? 'plus-add'
                  : isSelected
                  ? 'eye-on'
                  : 'eye-off'
              }
            />
          </Button>
        </RightSide>
      </StyledWrapper>
    )
  }
)
export default AssetWatchlistItem
