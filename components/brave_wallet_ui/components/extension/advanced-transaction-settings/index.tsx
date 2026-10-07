// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import { NavButton } from '../buttons/nav-button/index'
import { Panel } from '../panel/index'

// Styled Components
import {
  StyledWrapper,
  FormColumn,
  Input,
  InputLabel,
  ButtonRow,
  InfoText,
} from './style'

// Utils
import Amount from '../../../utils/amount'

export interface Props {
  onCancel: () => void
  nonce: string
  chainId: string
  txMetaId: string
  updateUnapprovedTransactionNonce: (payload: any) => void
}

export const AdvancedTransactionSettings = (props: Props) => {
  const {
    onCancel,
    nonce,
    chainId,
    txMetaId,
    updateUnapprovedTransactionNonce,
  } = props
  const [customNonce, setCustomNonce] = React.useState<string>(
    nonce && parseInt(nonce).toString(),
  )

  const handleNonceInputChanged = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    setCustomNonce(event.target.value)
  }

  const onSave = () => {
    updateUnapprovedTransactionNonce({
      chainId,
      txMetaId,
      nonce: customNonce && new Amount(customNonce).toHex(),
    })
    onCancel()
  }

  return (
    <Panel
      navAction={onCancel}
      title={getLocale(S.BRAVE_WALLET_ADVANCED_TRANSACTION_SETTINGS)}
    >
      <StyledWrapper>
        <FormColumn>
          <InputLabel
            textColor='secondary'
            variant='default.regular'
          >
            {getLocale(S.BRAVE_WALLET_EDIT_NONCE)}
          </InputLabel>
          <Input
            placeholder={getLocale(
              S.BRAVE_WALLET_ADVANCED_TRANSACTION_SETTINGS_PLACEHOLDER,
            )}
            type='number'
            value={customNonce}
            onChange={handleNonceInputChanged}
          />
          <InfoText
            textColor='tertiary'
            variant='small.regular'
          >
            {getLocale(S.BRAVE_WALLET_EDIT_GAS_ZERO_GAS_PRICE_WARNING)}
          </InfoText>
        </FormColumn>
        <ButtonRow>
          <NavButton
            buttonType='secondary'
            needsTopMargin={true}
            text={getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}
            onSubmit={onCancel}
          />
          <NavButton
            buttonType='primary'
            text={getLocale(S.BRAVE_WALLET_ACCOUNT_SETTINGS_SAVE)}
            onSubmit={onSave}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default AdvancedTransactionSettings
