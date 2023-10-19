// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Queries
import {
  useGetSelectedChainQuery //
} from '../../../../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import {
  SelectTokenOrNetworkButton //
} from '../../buttons/select-token-or-network/select-token-or-network'
import { SwapInput } from '../../inputs/swap-input/swap-input'

// Styled Components
import { LoadingRow } from './to-section.style'
import {
  Row,
  Column,
  Text,
  Loader,
  VerticalSpacer
} from '../../shared-swap.styles'

interface Props {
  onClickSelectToken: () => void
  onInputChange: (value: string) => void
  isLoading: boolean | undefined
  inputValue: string
  hasInputError: boolean
  token: BraveWallet.BlockchainToken | undefined
  disabled: boolean
}

export const ToSection = (props: Props) => {
  const {
    onClickSelectToken,
    onInputChange,
    token,
    inputValue,
    hasInputError,
    isLoading,
    disabled
  } = props

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  return (
    <Column columnWidth='full'>
      <LoadingRow
        rowWidth='full'
        horizontalAlign='flex-end'
      >
        {isLoading && (
          <>
            <Loader />
            <Text
              textSize='12px'
              textColor={hasInputError ? 'error' : 'text03'}
              isBold={false}
            >
              {getLocale('braveSwapFindingPrice')}
            </Text>
          </>
        )}
      </LoadingRow>

      <Row rowWidth='full'>
        <SelectTokenOrNetworkButton
          onClick={onClickSelectToken}
          network={selectedNetwork}
          asset={token}
          text={token?.symbol}
          buttonType='secondary'
          iconType='asset'
        />
        <SwapInput
          hasError={hasInputError}
          onChange={onInputChange}
          value={inputValue}
          disabled={disabled}
        />
      </Row>
      <VerticalSpacer size={6} />
    </Column>
  )
}
