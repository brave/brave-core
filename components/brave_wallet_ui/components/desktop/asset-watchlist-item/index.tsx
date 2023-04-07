// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { isDataURL } from '../../../utils/string-utils'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

// Components
import { withPlaceholderIcon } from '../../shared'
import { Checkbox } from 'brave-ui'
import { NftIcon } from '../../shared/nft-icon/nft-icon'

// Styled Components
import {
  StyledWrapper,
  AssetName,
  NameAndIcon,
  AssetIcon,
  DeleteButton,
  DeleteIcon,
  RightSide,
  NameAndSymbol,
  AssetSymbol
} from './style'

export interface Props {
  onSelectAsset: (key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => void
  onRemoveAsset: (token: BraveWallet.BlockchainToken) => void
  isCustom: boolean
  isSelected: boolean
  token: BraveWallet.BlockchainToken
}

const AssetWatchlistItem = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      onSelectAsset,
      onRemoveAsset,
      isCustom,
      token,
      isSelected
    } = props

    // queries
    const { data: tokensNetwork } = useGetNetworkQuery(token, { skip: !token })

    // callbacks
    const onCheck = React.useCallback((key: string, selected: boolean) => {
      onSelectAsset(key, selected, token, isCustom)
    }, [onSelectAsset, token, isCustom])

    const onClickAsset = React.useCallback(() => {
      onSelectAsset(token.contractAddress, !isSelected, token, isCustom)
    }, [onSelectAsset, token, isSelected, isCustom])

    const onClickRemoveAsset = React.useCallback(() => {
      onRemoveAsset(token)
    }, [token, onRemoveAsset])

    const AssetIconWithPlaceholder = React.useMemo(() => {
      return withPlaceholderIcon(token.isErc721 && !isDataURL(token.logo) ? NftIcon : AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
    }, [token])

    const networkDescription = React.useMemo(() => {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokensNetwork?.chainName ?? '')
    }, [tokensNetwork, token])

    return (
      <StyledWrapper ref={forwardedRef}>
        <NameAndIcon onClick={onClickAsset}>
          <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
          <NameAndSymbol>
            <AssetName>
              {token.name} {
                token.isErc721 && token.tokenId
                  ? '#' + new Amount(token.tokenId).toNumber()
                  : ''
              }
            </AssetName>
            <AssetSymbol>{networkDescription}</AssetSymbol>
          </NameAndSymbol>
        </NameAndIcon>
        <RightSide>
          {isCustom &&
            <DeleteButton onClick={onClickRemoveAsset}>
              <DeleteIcon />
            </DeleteButton>
          }
          <Checkbox value={{ [`${token.contractAddress}-${token.symbol}-${token.chainId}-${token.tokenId}`]: isSelected }} onChange={onCheck}>
            <div data-key={`${token.contractAddress}-${token.symbol}-${token.chainId}-${token.tokenId}`} />
          </Checkbox>
        </RightSide>
      </StyledWrapper>
    )
  }
)
export default AssetWatchlistItem
