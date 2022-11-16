// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { WalletSelectors } from '../../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'

// Assets
import CaratDownIcon from '../../assets/carat-down-icon.svg'

// Types
import { BraveWallet, SendOptionTypes } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { getTokensNetwork } from '../../../../../utils/network-utils'
import Amount from '../../../../../utils/amount'

// Components
import {
  withPlaceholderIcon,
  CreateNetworkIcon
} from '../../../../../components/shared'
import { NftIcon } from '../../../../../components/shared/nft-icon/nft-icon'

// Styled Components
import {
  AssetIcon,
  NetworkIconWrapper,
  Button,
  ButtonIcon,
  IconsWrapper
} from './select-token-button.style'
import { Row, Text } from '../../shared.styles'

interface Props {
  onClick: () => void
  token: BraveWallet.BlockchainToken | undefined
  selectedSendOption: SendOptionTypes
}

export const SelectTokenButton = (props: Props) => {
  const { onClick, token, selectedSendOption } = props

  // Wallet Selectors
  const networks = useUnsafeWalletSelector(WalletSelectors.networkList)

  // Memos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(token?.isErc721 ? NftIcon : AssetIcon, {
      size: 'big',
      marginLeft: 0,
      marginRight: 0
    })
  }, [token?.isErc721])

  const tokensNetwork = React.useMemo(() => {
    if (token) {
      return getTokensNetwork(networks, token)
    }
    return undefined
  }, [token, networks])

  const buttonText = React.useMemo(() => {
    if (selectedSendOption === 'nft') {
      const id = token?.tokenId ? `#${new Amount(token?.tokenId).toNumber()}` : ''
      return token !== undefined ? `${token.name} ${id}` : getLocale('braveWalletSelectNFT')
    }
    return token !== undefined ? token.symbol : getLocale('braveWalletSelectToken')
  }, [selectedSendOption, token])

  return (
    <Button onClick={onClick} morePadding={token !== undefined} isNFT={selectedSendOption === 'nft'}>
      <Row>
        {token && (
          <IconsWrapper>
            <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
            {tokensNetwork && token?.contractAddress !== '' && (
              <NetworkIconWrapper>
                <CreateNetworkIcon network={tokensNetwork} marginRight={0} />
              </NetworkIconWrapper>
            )}
          </IconsWrapper>
        )}
        <Text
          isBold={token !== undefined}
          textColor={token !== undefined ? 'text01' : 'text03'}
          textSize={token !== undefined ? '18px' : '16px'}
        >
          {buttonText}
        </Text>
      </Row>
      <ButtonIcon size={12} icon={CaratDownIcon} />
    </Button>
  )
}

export default SelectTokenButton
