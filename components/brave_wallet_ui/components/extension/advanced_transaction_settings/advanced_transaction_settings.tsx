// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  LabelText,
  DescriptionText,
  Input,
  ButtonText,
} from './advanced_transaction_settings.styles'
import { Column, Row } from '../../shared/style'

export interface Props {
  nonce: string
  onCancel: () => void
  onSave: (nonce: string) => void
}

export const AdvancedTransactionSettings = (props: Props) => {
  const { nonce, onCancel, onSave } = props

  // State
  const [customNonce, setCustomNonce] = React.useState<string>(
    nonce && parseInt(nonce).toString(),
  )

  // Methods
  const handleNonceInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    setCustomNonce(event.target.value)
  }

  const onClickSave = React.useCallback(() => {
    onSave(customNonce)
    onCancel()
  }, [customNonce, onSave, onCancel])

  return (
    <StyledWrapper
      gap='16px'
      padding='16px'
      width='100%'
    >
      <Column
        gap='4px'
        width='100%'
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <LabelText textColor='primary'>
          {getLocale(S.BRAVE_WALLET_EDIT_NONCE)}
        </LabelText>
        <Input
          placeholder={getLocale(
            S.BRAVE_WALLET_ADVANCED_TRANSACTION_SETTINGS_PLACEHOLDER,
          )}
          type='number'
          value={customNonce}
          onChange={handleNonceInputChanged}
        />
        <DescriptionText textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_EDIT_GAS_ZERO_GAS_PRICE_WARNING)}
        </DescriptionText>
      </Column>
      <Row gap='16px'>
        <Button
          kind='outline'
          size='medium'
          onClick={onCancel}
        >
          <ButtonText>{getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}</ButtonText>
        </Button>
        <Button
          size='medium'
          onClick={onClickSave}
          isDisabled={customNonce === nonce || customNonce === ''}
        >
          <ButtonText>
            {getLocale(S.BRAVE_WALLET_ACCOUNT_SETTINGS_SAVE)}
          </ButtonText>
        </Button>
      </Row>
    </StyledWrapper>
  )
}

export default AdvancedTransactionSettings
