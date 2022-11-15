// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Radio from 'brave-ui/components/formControls/radio'

// Utils
import { getLocale } from '$web-common/locale'

// Styled Components
import { NavButton, Panel } from '../'
import {
  StyledWrapper,
  FormColumn,
  Input,
  ButtonRow,
  Description,
  AllowanceTitle,
  AllowanceContent,
  AllowanceOption
} from './style'

type AllowanceTypes =
  | 'proposed'
  | 'custom'

export interface Props {
  onCancel: () => void
  onSave: (allowance: string) => void
  proposedAllowance: string
  symbol: string
  approvalTarget: string
  isApprovalUnlimited: boolean
}

const EditAllowance = (props: Props) => {
  const [allowanceType, setAllowanceType] = React.useState<AllowanceTypes>('proposed')
  const [customAllowance, setCustomAllowance] = React.useState<string>('')

  const {
    onCancel,
    onSave,
    proposedAllowance,
    approvalTarget,
    symbol,
    isApprovalUnlimited
  } = props

  const toggleAllowanceRadio = (key: AllowanceTypes) => {
    setAllowanceType(key)
  }

  const onChangeCustomAllowance = (event: React.ChangeEvent<HTMLInputElement>) => {
    setCustomAllowance(event.target.value)
  }

  const onClickSave = () => {
    onSave(allowanceType === 'custom' ? customAllowance : proposedAllowance)
    onCancel()
  }

  const isSaveButtonDisabled = React.useMemo(() => {
    return allowanceType === 'custom' && customAllowance === ''
  }, [allowanceType, customAllowance])

  const formattedProposedAllowance = React.useMemo(() => {
    return isApprovalUnlimited
      ? getLocale('braveWalletTransactionApproveUnlimited')
      : proposedAllowance
  }, [proposedAllowance, isApprovalUnlimited])

  return (
    <Panel
      navAction={onCancel}
      title={getLocale('braveWalletEditPermissionsTitle')}
    >
      <StyledWrapper>
        <Description>
          {getLocale('braveWalletEditPermissionsDescription').replace('$1', approvalTarget)}
        </Description>
        <FormColumn>
          <Radio
            value={{
              proposed: allowanceType === 'proposed',
              custom: allowanceType === 'custom'
            }}
            onChange={toggleAllowanceRadio}
          >
            <div data-value='proposed'>
              <AllowanceOption>
                <AllowanceTitle>
                  {getLocale('braveWalletEditPermissionsProposedAllowance')}
                </AllowanceTitle>
                <AllowanceContent>
                  {formattedProposedAllowance} {symbol}
                </AllowanceContent>
              </AllowanceOption>
            </div>
            <div data-value='custom'>
              <AllowanceOption>
                <AllowanceTitle>
                  {getLocale('braveWalletEditPermissionsCustomAllowance')}
                </AllowanceTitle>
                <Input
                  placeholder={`0 ${symbol}`}
                  type='number'
                  value={customAllowance}
                  onChange={onChangeCustomAllowance}
                />
              </AllowanceOption>
            </div>
          </Radio>
        </FormColumn>

        <ButtonRow>
          <NavButton
            buttonType='secondary'
            text={getLocale('braveWalletButtonCancel')}
            onSubmit={onCancel}
          />
          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletAccountSettingsSave')}
            onSubmit={onClickSave}
            disabled={isSaveButtonDisabled}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default EditAllowance
