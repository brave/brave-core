// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import { useExplorer } from '../../../../common/hooks/explorer'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import { reduceAddress } from '../../../../utils/reduce-address'
import { getNFTTokenStandard } from '../../../../utils/string-utils'
import Amount from '../../../../utils/amount'

// Styled Components
import { Wrapper, Tip, TipIcon, AddressLink } from './nft_info_tooltip.style'
import {
  Text,
  Column,
  VerticalSpacer
} from '../../../../components/shared/style'

export interface Props {
  token: BraveWallet.BlockchainToken
  network: BraveWallet.NetworkInfo | undefined
}

export const NFTInfoTooltip = (props: Props) => {
  const { token, network } = props

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(
    network || new BraveWallet.NetworkInfo()
  )

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
      <TipIcon />
      {active && (
        <Tip>
          <Column
            fullWidth={true}
            margin='0px 0px 18px 0px'
            alignItems='flex-start'
          >
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text01'
              isBold={true}
            >
              {getLocale('braveWalletNFTDetailContractAddress')}
            </Text>
            <VerticalSpacer space={8} />
            <AddressLink
              onClick={onClickViewOnBlockExplorer(
                'address',
                token.contractAddress
              )}
            >
              {reduceAddress(token.contractAddress)}
            </AddressLink>
          </Column>
          {token.coin !== BraveWallet.CoinType.SOL && (
            <Column
              fullWidth={true}
              margin='0px 0px 18px 0px'
              alignItems='flex-start'
            >
              <Text
                textAlign='left'
                textSize='14px'
                textColor='text01'
                isBold={true}
              >
                {getLocale('braveWalletNFTDetailTokenID')}
              </Text>
              <VerticalSpacer space={8} />
              <AddressLink
                onClick={onClickViewOnBlockExplorer(
                  'nft',
                  token.contractAddress,
                  token.tokenId
                )}
              >
                {'#' + new Amount(token.tokenId).toNumber()}
              </AddressLink>
            </Column>
          )}
          <Column
            fullWidth={true}
            margin='0px 0px 18px 0px'
            alignItems='flex-start'
          >
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text01'
              isBold={true}
            >
              {getLocale('braveWalletNFTDetailBlockchain')}
            </Text>
            <VerticalSpacer space={8} />
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text02'
            >
              {network?.chainName}
            </Text>
          </Column>
          <Column
            fullWidth={true}
            alignItems='flex-start'
          >
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text01'
              isBold={true}
            >
              {getLocale('braveWalletNFTDetailTokenStandard')}
            </Text>
            <VerticalSpacer space={8} />
            <Text
              textAlign='left'
              textSize='14px'
              textColor='text02'
            >
              {getNFTTokenStandard(token)}
            </Text>
          </Column>
        </Tip>
      )}
    </Wrapper>
  )
}

export default NFTInfoTooltip
