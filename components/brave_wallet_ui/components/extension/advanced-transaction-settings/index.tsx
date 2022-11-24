// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import { NavButton, Panel } from '../'

// Styled Components
import {
  StyledWrapper,
  FormColumn,
  Input,
  InputLabel,
  ButtonRow,
  InfoText
} from './style'

// Utils
import Amount from '../../../utils/amount'

export interface Props {
  onCancel: () => void
  nonce: string
  txMetaId: string
  updateUnapprovedTransactionNonce: (payload: any) => void
}

const AdvancedTransactionSettings = (props: Props) => {
  const {
    onCancel,
    nonce,
    txMetaId,
    updateUnapprovedTransactionNonce
  } = props
  const [customNonce, setCustomNonce] = React.useState<string>(
    nonce && parseInt(nonce).toString()
  )

  const handleNonceInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setCustomNonce(event.target.value)
  }

  const onSave = () => {
    updateUnapprovedTransactionNonce({
      txMetaId,
      nonce: customNonce && new Amount(customNonce).toHex()
    })
    onCancel()
  }

  return (
    <Panel
      navAction={onCancel}
      title={getLocale('braveWalletAdvancedTransactionSettings')}
    >
      <StyledWrapper>
        <FormColumn>
          <InputLabel>{getLocale('braveWalletEditNonce')}</InputLabel>
          <Input
            placeholder={getLocale('braveWalletAdvancedTransactionSettingsPlaceholder')}
            type='number'
            value={customNonce}
            onChange={handleNonceInputChanged}
          />
          <InfoText>{getLocale('braveWalletEditGasZeroGasPriceWarning')}</InfoText>
        </FormColumn>
        <ButtonRow>
          <NavButton
            buttonType='secondary'
            needsTopMargin={true}
            text={getLocale('braveWalletButtonCancel')}
            onSubmit={onCancel}
          />
          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletAccountSettingsSave')}
            onSubmit={onSave}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default AdvancedTransactionSettings
