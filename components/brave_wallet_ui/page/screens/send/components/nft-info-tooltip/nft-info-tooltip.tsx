// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import { useExplorer } from '../../../../../common/hooks'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Assets
import InfoIcon from '../../../../../assets/svg-icons/info-icon.svg'

// Utils
import { getLocale } from '../../../../../../common/locale'
import { reduceAddress } from '../../../../../utils/reduce-address'
import { getNFTTokenStandard } from '../../../../../utils/string-utils'
import Amount from '../../../../../utils/amount'

// Styled Components
import {
  Wrapper,
  Tip,
  TipIcon,
  AddressLink
} from './nft-info-tooltip.style'
import { Text, Column, VerticalSpacer } from '../../shared.styles'

export interface Props {
  token: BraveWallet.BlockchainToken
  network: BraveWallet.NetworkInfo | undefined
}

export const NFTInfoTooltip = (props: Props) => {
  const { token, network } = props

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(network || new BraveWallet.NetworkInfo())

  // State
  const [active, setActive] = React.useState(false)

  // Methods
  const showTip = () => {
    setActive(true)
  }

  const hideTip = () => {
    setActive(false)
  }

  return (
    <Wrapper
      onMouseEnter={showTip}
      onMouseLeave={hideTip}
    >
      <TipIcon icon={InfoIcon} size={14} />
      {active &&
        <Tip>
          <Column marginBottom={18} columnWidth='full' horizontalAlign='flex-start'>
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text01'
              isBold={true}
            >
              {getLocale('braveWalletNFTDetailContractAddress')}
            </Text>
            <VerticalSpacer size={8} />
            <AddressLink
              onClick={onClickViewOnBlockExplorer('address', token.contractAddress)}
            >
              {reduceAddress(token.contractAddress)}
            </AddressLink>
          </Column>
          {token.coin !== BraveWallet.CoinType.SOL &&
            <Column marginBottom={18} columnWidth='full' horizontalAlign='flex-start'>
              <Text
                textAlign='left'
                textSize='14px'
                textColor='text01'
                isBold={true}
              >
                {getLocale('braveWalletNFTDetailTokenID')}
              </Text>
              <VerticalSpacer size={8} />
              <AddressLink
                onClick={onClickViewOnBlockExplorer('nft', token.contractAddress, token.tokenId)}
              >
                {'#' + new Amount(token.tokenId).toNumber()}
              </AddressLink>
            </Column>
          }
          <Column marginBottom={18} columnWidth='full' horizontalAlign='flex-start'>
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text01'
              isBold={true}
            >
              {getLocale('braveWalletNFTDetailBlockchain')}
            </Text>
            <VerticalSpacer size={8} />
            <Text textAlign='left' textSize='14px' textColor='text02'>
              {network?.chainName}
            </Text>
          </Column>
          <Column columnWidth='full' horizontalAlign='flex-start'>
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text01'
              isBold={true}
            >
              {getLocale('braveWalletNFTDetailTokenStandard')}
            </Text>
            <VerticalSpacer size={8} />
            <Text textAlign='left' textSize='14px' textColor='text02'>
              {getNFTTokenStandard(token)}
            </Text>
          </Column>
        </Tip>
      }
    </Wrapper>
  )
}

export default NFTInfoTooltip
