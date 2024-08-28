// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// Queries
import {
  useGetNetworkQuery //
} from '../../../../../common/slices/api.slice'

// Types
import { MeldCryptoCurrency } from '../../../../../constants/types'

// Utils
import {
  getAssetSymbol,
  getMeldTokensCoinType
} from '../../../../../utils/meld_utils'

// Components
import {
  CreateNetworkIcon //
} from '../../../../../components/shared/create-network-icon'

// Styled Components
import { Column, Row } from '../../../../../components/shared/style'
import {
  AssetIcon,
  CaretDown,
  ControlText,
  Label,
  WrapperButton,
  IconsWrapper,
  NetworkIconWrapper
} from '../shared/style'

interface SelectAssetButtonProps {
  labelText: string
  selectedAsset?: MeldCryptoCurrency
  onClick: () => void
}

export const SelectAssetButton = (props: SelectAssetButtonProps) => {
  const { labelText, selectedAsset, onClick } = props

  // Queries
  const { data: tokensNetwork } = useGetNetworkQuery(
    selectedAsset?.chainId
      ? {
          chainId: selectedAsset.chainId,
          coin: getMeldTokensCoinType(selectedAsset)
        }
      : skipToken
  )

  // Computed
  const assetSymbol = selectedAsset ? getAssetSymbol(selectedAsset) : ''

  return (
    <Column alignItems='flex-start'>
      <WrapperButton onClick={onClick}>
        <Column alignItems='flex-start'>
          <Label>{labelText}</Label>
          <Row
            justifyContent='flex-start'
            gap='8px'
            minWidth='94px'
            minHeight='40px'
          >
            {selectedAsset && (
              <>
                <IconsWrapper>
                  <AssetIcon
                    size='40px'
                    src={`chrome://image?${selectedAsset?.symbolImageUrl}`}
                  />
                  {tokensNetwork && (
                    <NetworkIconWrapper>
                      <CreateNetworkIcon
                        network={tokensNetwork}
                        marginRight={0}
                        size='tiny'
                      />
                    </NetworkIconWrapper>
                  )}
                </IconsWrapper>
                <ControlText>{assetSymbol}</ControlText>
              </>
            )}
            <CaretDown />
          </Row>
        </Column>
      </WrapperButton>
    </Column>
  )
}
