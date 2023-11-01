// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Queries
import { useGetSelectedChainQuery } from '../../../../../../common/slices/api.slice'
import { useSelectedAccountQuery } from '../../../../../../common/slices/api.slice.extra'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import { SwapSectionBox } from '../../swap-section-box/swap-section-box'
import {
  SelectTokenOrNetworkButton //
} from '../../buttons/select-token-or-network/select-token-or-network'
import { PresetButton } from '../../buttons/preset-button/preset-button'
import { SwapInput } from '../../inputs/swap-input/swap-input'

// Styled Components
import {
  Row,
  Column,
  HorizontalDivider,
  Text,
  HiddenResponsiveRow
} from '../../shared-swap.styles'

interface Props {
  onClickSelectToken: () => void
  onInputChange: (value: string) => void
  inputValue: string
  hasInputError: boolean
  token: BraveWallet.BlockchainToken | undefined
  tokenBalance: Amount
  fiatValue: string | undefined
}

export const FromSection = (props: Props) => {
  const {
    token,
    onClickSelectToken,
    onInputChange,
    hasInputError,
    inputValue,
    tokenBalance,
    fiatValue
  } = props

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()

  // methods
  const onClickHalfPreset = () => {
    if (!token) {
      return
    }
    onInputChange(
      tokenBalance
        .div(2)
        .parseInteger()
        .divideByDecimals(token.decimals)
        .format(6)
    )
  }

  const onClickMaxPreset = () => {
    if (!token) {
      return
    }
    onInputChange(tokenBalance.divideByDecimals(token.decimals).format())
  }

  // render
  return (
    <SwapSectionBox boxType='primary'>
      <Column
        columnWidth='full'
        columnHeight='full'
      >
        <Row
          rowWidth='full'
          horizontalAlign='flex-end'
        >
          {token && (
            <Text
              textSize='14px'
              textColor={hasInputError ? 'error' : 'text02'}
              maintainHeight={true}
            >
              {!tokenBalance.isUndefined()
                ? `${getLocale('braveSwapBalance')} ${tokenBalance
                    .divideByDecimals(token.decimals)
                    .format(6)}`
                : ''}
            </Text>
          )}
        </Row>
        <Row
          rowWidth='full'
          verticalAlign='center'
        >
          <Row>
            <SelectTokenOrNetworkButton
              onClick={onClickSelectToken}
              asset={token}
              network={selectedNetwork}
              text={token?.symbol}
              buttonType='primary'
              iconType='asset'
            />
            {token && selectedAccount !== undefined && (
              <Row>
                <HorizontalDivider
                  height={28}
                  marginLeft={8}
                  marginLeftResponsive={6}
                  marginRight={8}
                />
                <HiddenResponsiveRow maxWidth={570}>
                  <PresetButton
                    buttonText={getLocale('braveSwapHalf')}
                    onClick={onClickHalfPreset}
                  />
                </HiddenResponsiveRow>
                <PresetButton
                  buttonText={getLocale('braveSwapMax')}
                  onClick={onClickMaxPreset}
                />
              </Row>
            )}
          </Row>
          <SwapInput
            onChange={onInputChange}
            value={inputValue}
            hasError={hasInputError}
            autoFocus={true}
          />
        </Row>
        <Row
          rowWidth='full'
          horizontalAlign='flex-end'
        >
          {token && (
            <Text
              textSize='14px'
              textColor='text03'
              maintainHeight={true}
            >
              {fiatValue}
            </Text>
          )}
        </Row>
      </Column>
    </SwapSectionBox>
  )
}
