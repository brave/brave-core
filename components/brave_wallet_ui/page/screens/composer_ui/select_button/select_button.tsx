// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet, SendPageTabHashes } from '../../../../constants/types'

// Utils
import { checkIfTokenNeedsNetworkIcon } from '../../../../utils/asset-utils'
import Amount from '../../../../utils/amount'

// Hooks
import { useGetNetworkQuery } from '../../../../common/slices/api.slice'

// Components
import {
  withPlaceholderIcon //
} from '../../../../components/shared/create-placeholder-icon/index'
import {
  CreateNetworkIcon //
} from '../../../../components/shared/create-network-icon/index'
import { NftIcon } from '../../../../components/shared/nft-icon/nft-icon'

// Styled Components
import {
  AssetIcon,
  NetworkIconWrapper,
  Button,
  ButtonIcon,
  IconsWrapper,
  ButtonText
} from './select_button.style'
import { Row } from '../../../../components/shared/style'

interface Props {
  onClick: () => void
  token: BraveWallet.BlockchainToken | undefined
  selectedSendOption: SendPageTabHashes
  placeholderText: string
  disabled?: boolean
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const SelectButton = (props: Props) => {
  const { onClick, token, selectedSendOption, placeholderText, disabled } =
    props

  // Queries
  const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)

  // Memos
  const buttonText = React.useMemo(() => {
    if (selectedSendOption === SendPageTabHashes.nft) {
      const id = token?.tokenId
        ? `#${new Amount(token?.tokenId).toNumber()}`
        : ''
      return token !== undefined ? `${token.name} ${id}` : placeholderText
    }
    return token !== undefined ? token.symbol : placeholderText
  }, [selectedSendOption, token, placeholderText])

  return (
    <Button
      onClick={onClick}
      morePadding={token !== undefined}
      isNFT={selectedSendOption === SendPageTabHashes.nft}
      disabled={disabled}
    >
      <Row>
        {token && (
          <IconsWrapper>
            {token.isNft || token.isErc721 ? (
              <NftIconWithPlaceholder asset={token} />
            ) : (
              <AssetIconWithPlaceholder asset={token} />
            )}
            {tokensNetwork &&
              checkIfTokenNeedsNetworkIcon(
                tokensNetwork,
                token.contractAddress
              ) && (
                <NetworkIconWrapper>
                  <CreateNetworkIcon
                    network={tokensNetwork}
                    marginRight={0}
                  />
                </NetworkIconWrapper>
              )}
          </IconsWrapper>
        )}
        <ButtonText
          textSize='22px'
          isNFT={selectedSendOption === SendPageTabHashes.nft}
          textAlign='left'
          isPlaceholder={!token}
        >
          {buttonText}
        </ButtonText>
      </Row>
      <ButtonIcon />
    </Button>
  )
}
