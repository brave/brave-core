// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import {
  PasswordInput
} from '../../shared/password-input/password-input-v2'

// Styled Components
import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  InputColumn,
  UnlockButton,
  InputLabel
} from './style'

import {
  VerticalSpace,
  Row
} from '../../shared/style'

interface Props {
  value?: string
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onShowRestore: () => void
  hasPasswordError: boolean
  disabled: boolean
}

export const LockScreen = (props: Props) => {
  const {
    value,
    onSubmit,
    onPasswordChanged,
    onShowRestore,
    disabled,
    hasPasswordError
  } = props

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  return (
    <StyledWrapper>
      <PageIcon />
      <Title>
        {getLocale('braveWalletUnlockWallet')}
      </Title>
      <Description
        textSize='16px'
      >
        {getLocale('braveWalletLockScreenTitle')}
      </Description>
      <InputColumn
        fullWidth={true}
      >
        <Row
          justifyContent='flex-start'
          padding='0px 4px'
          marginBottom={4}
        >
          <InputLabel
            textSize='12px'
            isBold={true}
          >
            {getLocale('braveWalletInputLabelPassword')}
          </InputLabel>
        </Row>
        <PasswordInput
          placeholder={
            getLocale('braveWalletEnterYourPassword')
          }
          onChange={onPasswordChanged}
          onKeyDown={handleKeyDown}
          error={
            getLocale('braveWalletLockScreenError')
          }
          hasError={hasPasswordError}
          autoFocus={true}
          value={value}
        />
        <VerticalSpace space='24px' />
        <UnlockButton
          onClick={onSubmit}
          isDisabled={disabled}
          kind='filled'
          size='large'
        >
          {getLocale('braveWalletLockScreenButton')}
        </UnlockButton>
        <Button
          onClick={onShowRestore}
          kind='plain'
        >
          {getLocale('braveWalletWelcomeRestoreButton')}
        </Button>
      </InputColumn>
    </StyledWrapper>
  )
}

export default LockScreen
