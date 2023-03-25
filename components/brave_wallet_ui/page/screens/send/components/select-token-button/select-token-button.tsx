// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Assets
import CaratDownIcon from '../../assets/carat-down-icon.svg'

// Types
import { BraveWallet, SendOptionTypes } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { checkIfTokenNeedsNetworkIcon } from '../../../../../utils/asset-utils'
import Amount from '../../../../../utils/amount'

// Hooks
import { useGetNetworkQuery } from '../../../../../common/slices/api.slice'

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
  IconsWrapper,
  ButtonText
} from './select-token-button.style'
import { Row } from '../../shared.styles'

interface Props {
  onClick: () => void
  token: BraveWallet.BlockchainToken | undefined
  selectedSendOption: SendOptionTypes
}

export const SelectTokenButton = (props: Props) => {
  const { onClick, token, selectedSendOption } = props

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
          <IconsWrapper marginRight={selectedSendOption === 'nft' ? 12 : undefined}>
            <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
            {
              tokensNetwork &&
              checkIfTokenNeedsNetworkIcon(tokensNetwork, token.contractAddress) &&
              <NetworkIconWrapper>
                <CreateNetworkIcon network={tokensNetwork} marginRight={0} />
              </NetworkIconWrapper>
            }
          </IconsWrapper>
        )}
        <ButtonText
          isBold={token !== undefined}
          textColor={token !== undefined ? 'text01' : 'text03'}
          textSize={token !== undefined ? '18px' : '16px'}
          isNFT={selectedSendOption === 'nft'}
          textAlign='left'
        >
          {buttonText}
        </ButtonText>
      </Row>
      <ButtonIcon size={12} icon={CaratDownIcon} />
    </Button>
  )
}

export default SelectTokenButton
