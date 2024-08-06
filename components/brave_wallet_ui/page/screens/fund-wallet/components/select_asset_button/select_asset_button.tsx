// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { MeldCryptoCurrency } from '../../../../../constants/types'

// styles
import { Column, Row } from '../../../../../components/shared/style'
import {
  AssetIcon,
  CaretDown,
  ControlsWrapper,
  ControlText,
  Label,
  WrapperButton
} from '../shared/style'
import { getAssetSymbol } from '../../../../../utils/meld_utils'

interface SelectAssetButtonProps {
  labelText: string
  selectedAsset?: MeldCryptoCurrency
  onClick: () => void
}

export const SelectAssetButton = (props: SelectAssetButtonProps) => {
  const { labelText, selectedAsset, onClick } = props

  const assetSymbol = selectedAsset ? getAssetSymbol(selectedAsset) : ''

  return (
    <Column alignItems='flex-start'>
      <WrapperButton onClick={onClick}>
        <Column alignItems='flex-start'>
          <Label>{labelText}</Label>
          <ControlsWrapper>
            <Row
              justifyContent='flex-start'
              gap='8px'
              minWidth='94px'
              minHeight='40px'
            >
              {selectedAsset && (
                <>
                  <AssetIcon
                    size='40px'
                    src={`chrome://image?${selectedAsset?.symbolImageUrl}`}
                  />
                  <ControlText>{assetSymbol}</ControlText>
                </>
              )}
            </Row>
            <CaretDown />
          </ControlsWrapper>
        </Column>
      </WrapperButton>
    </Column>
  )
}
