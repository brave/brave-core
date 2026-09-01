// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation } from 'react-router-dom'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet, WalletRoutes } from '$wallet/constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import Amount from '$wallet/utils/amount'
import { useGetNetworkQuery } from '$wallet/common/slices/api.slice'
import { reduceInt } from '$wallet/utils/string-utils'

// Components
import {
  withPlaceholderIcon, //
} from '$wallet/components/shared/create-placeholder-icon/index'

// Styled Components
import {
  StyledWrapper,
  NameAndIcon,
  AssetIcon,
  NftIconWrapper,
  Button,
  Icon,
  RightSide,
  NameAndSymbol,
} from './visible_asset_item.style'
import { HorizontalSpace, Text } from '$wallet/components/shared/style'

export interface Props {
  onSelectAsset: (token: BraveWallet.BlockchainToken) => void
  onRemoveAsset: (token: BraveWallet.BlockchainToken) => void
  isRemovable: boolean
  isSelected: boolean
  token: BraveWallet.BlockchainToken
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 8 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIconWrapper, ICON_CONFIG)

export const VisibleAssetItem = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const { onSelectAsset, onRemoveAsset, isRemovable, token, isSelected } =
      props

    // routing
    const { hash } = useLocation()

    // queries
    const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)

    // callbacks
    const onClickAsset = React.useCallback(() => {
      onSelectAsset(token)
    }, [onSelectAsset, token])

    const onClickRemoveAsset = React.useCallback(() => {
      onRemoveAsset(token)
    }, [token, onRemoveAsset])

    // computed
    const isVisible = isSelected
      ? !token.visible // pending visibility change
      : token.visible

    return (
      <StyledWrapper ref={forwardedRef}>
        <NameAndIcon>
          {token.isNft ? (
            <NftIconWithPlaceholder asset={token} />
          ) : (
            <AssetIconWithPlaceholder asset={token} />
          )}
          <NameAndSymbol>
            <Text
              textColor='primary'
              variant='default.semibold'
              textAlign='left'
            >
              {token.name || token.symbol}{' '}
              {token.isErc721 && token.tokenId
                ? '#' + reduceInt(new Amount(token.tokenId).format())
                : ''}
            </Text>
            <Text
              textColor='primary'
              variant='small.regular'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_PORTFOLIO_ASSET_NETWORK_DESCRIPTION)
                .replace('$1', token.symbol)
                .replace('$2', tokensNetwork?.chainName ?? '')}
            </Text>
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
          <Button onClick={onClickAsset}>
            <Icon
              name={
                hash === WalletRoutes.AvailableAssetsHash
                  ? 'plus-add'
                  : isVisible
                    ? 'eye-on'
                    : 'eye-off'
              }
            />
          </Button>
        </RightSide>
      </StyledWrapper>
    )
  },
)
