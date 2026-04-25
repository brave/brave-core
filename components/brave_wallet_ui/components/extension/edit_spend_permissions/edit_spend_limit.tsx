// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import RadioButton from '@brave/leo/react/radioButton'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  LabelText,
  AmountText,
  Input,
  ButtonText,
} from './edit_spend_limit.style'
import { Column, Row } from '../../shared/style'

type AllowanceTypes = 'proposed' | 'custom'

export interface Props {
  onCancel: () => void
  onSave: (limit: string) => void
  proposedAllowance: string
  symbol: string
  approvalTarget: string
  isApprovalUnlimited: boolean
}

export const EditSpendLimit = (props: Props) => {
  const {
    onCancel,
    onSave,
    approvalTarget,
    isApprovalUnlimited,
    proposedAllowance,
    symbol,
  } = props

  // State
  const [allowanceType, setAllowanceType] =
    React.useState<AllowanceTypes>('proposed')
  const [customAllowance, setCustomAllowance] = React.useState<string>('')

  // Refs
  const customAllowanceInputRef = React.useRef<HTMLInputElement>(null)

  // Methods
  const toggleAllowanceRadio = (key: AllowanceTypes) => {
    if (key === 'custom') {
      customAllowanceInputRef.current?.focus()
    }
    setAllowanceType(key)
  }

  const onChangeCustomAllowance = (
    event: React.ChangeEvent<HTMLInputElement>,
  ) => {
    setCustomAllowance(event.target.value)
  }

  const onClickSave = React.useCallback(() => {
    onSave(allowanceType === 'custom' ? customAllowance : proposedAllowance)
    onCancel()
  }, [allowanceType, customAllowance, proposedAllowance, onSave, onCancel])

  // Memos
  const isSaveButtonDisabled = React.useMemo(() => {
    return allowanceType === 'custom' && customAllowance === ''
  }, [allowanceType, customAllowance])

  const formattedProposedAllowance = React.useMemo(() => {
    return isApprovalUnlimited
      ? getLocale('braveWalletTransactionApproveUnlimited')
      : proposedAllowance
  }, [proposedAllowance, isApprovalUnlimited])

  return (
    <StyledWrapper
      gap='32px'
      padding='16px'
      width='100%'
    >
      <LabelText
        textColor='secondary'
        textAlign='left'
      >
        {getLocale('braveWalletEditPermissionsDescription').replace(
          '$1',
          approvalTarget,
        )}
      </LabelText>
      <Column
        gap='16px'
        width='100%'
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <RadioButton
          name='proposed'
          value='proposed'
          currentValue={allowanceType}
          onChange={() => toggleAllowanceRadio('proposed')}
        >
          <Column
            alignItems='flex-start'
            justifyContent='flex-start'
          >
            <LabelText textColor='primary'>
              {getLocale('braveWalletProposedSpendLimit')}
            </LabelText>
            <AmountText textColor='primary'>
              {formattedProposedAllowance} {symbol}
            </AmountText>
          </Column>
        </RadioButton>
        <RadioButton
          name='custom'
          value='custom'
          currentValue={allowanceType}
          onChange={() => toggleAllowanceRadio('custom')}
        >
          <Column
            alignItems='flex-start'
            justifyContent='flex-start'
            gap='4px'
          >
            <LabelText textColor='primary'>
              {getLocale('braveWalletCustomSpendLimit')}
            </LabelText>
            <Input
              ref={customAllowanceInputRef}
              placeholder='0'
              type='number'
              value={customAllowance}
              onChange={onChangeCustomAllowance}
              data-testid='custom-allowance-input'
            />
          </Column>
        </RadioButton>
      </Column>
      <Row gap='16px'>
        <Button
          kind='outline'
          size='medium'
          onClick={onCancel}
        >
          <ButtonText>{getLocale('braveWalletButtonCancel')}</ButtonText>
        </Button>
        <Button
          size='medium'
          onClick={onClickSave}
          isDisabled={isSaveButtonDisabled}
        >
          <ButtonText>{getLocale('braveWalletAccountSettingsSave')}</ButtonText>
        </Button>
      </Row>
    </StyledWrapper>
  )
}

export default EditSpendLimit
