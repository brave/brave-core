// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../constants/types'

// utils
import Amount from '../../utils/amount'
import { getLocale } from '../../../common/locale'

// components
import { MediumAssetIcon } from '../shared/style'
import { withPlaceholderIcon, CreateNetworkIcon } from '../shared'

// styles
import {
  BuyAssetOptionWrapper,
  AssetName,
  IconsWrapper,
  NameAndIcon,
  NameColumn,
  NetworkDescriptionText,
  NetworkIconWrapper
} from './buy-asset-option.styles'

interface Props {
  onClick?: (token: BraveWallet.BlockchainToken) => void
  token: BraveWallet.BlockchainToken
  tokenNetwork: BraveWallet.NetworkInfo
  isSelected?: boolean
  isPanel?: boolean
}

const AssetIconWithPlaceholder = withPlaceholderIcon(MediumAssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })

export const BuyAssetOptionItem = ({
  onClick,
  token,
  tokenNetwork,
  isSelected,
  isPanel
}: Props) => {
  // memos
  const networkDescription = React.useMemo(() => {
    if (tokenNetwork && !isPanel) {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokenNetwork.chainName ?? '')
    }
    return token.symbol
  }, [tokenNetwork, token])

  // methods
  const handleOnClick = React.useCallback(() => {
    if (onClick) {
      onClick(token)
    }
  }, [onClick, token])

  // render
  return (
    <>
      {token.visible &&
        <BuyAssetOptionWrapper isSelected={isSelected} onClick={handleOnClick}>
          <NameAndIcon>
            <IconsWrapper>
              <AssetIconWithPlaceholder asset={token} network={tokenNetwork} />
              {tokenNetwork && token.contractAddress !== '' && !isPanel &&
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
        </BuyAssetOptionWrapper>
      }
    </>
  )
}

export default BuyAssetOptionItem
