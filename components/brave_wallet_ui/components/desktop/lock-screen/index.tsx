// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

import Button from '@brave/leo/react/button'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../common/constants/local-storage-keys'
import { WalletRoutes } from '../../../constants/types'

// Utils
import { loadTimeData } from '../../../../common/loadTimeData'
import { getLocale } from '../../../../common/locale'
import { openWalletRouteTab } from '../../../utils/routes-utils'
import { UISelectors } from '../../../common/selectors'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { useUnlockWalletMutation } from '../../../common/slices/api.slice'
import getWalletAPIProxy from '../../../common/async/bridge'

// Components
import { PasswordInput } from '../../shared/password-input/password-input-v2'

// Styled Components
import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  InputColumn,
  UnlockButton,
  InputLabel,
  DoubleTapIcon,
  AndroidLockScreenWrapper,
  BraveLogo
} from './style'
import { VerticalSpace, Row, Text } from '../../shared/style'

export const LockScreen = () => {
  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // routing
  const history = useHistory()

  // state
  const [password, setPassword] = React.useState('')
  const [hasIncorrectPassword, setHasIncorrectPassword] = React.useState(false)

  // mutations
  const [attemptUnlockWallet] = useUnlockWalletMutation()

  // computed
  const disabled = password === ''

  // methods
  const unlockWallet = React.useCallback(async () => {
    const success = await attemptUnlockWallet(password).unwrap()
    setPassword('')
    if (success) {
      const sessionRoute = window.localStorage.getItem(
        LOCAL_STORAGE_KEYS.SAVED_SESSION_ROUTE
      )
      history.push(sessionRoute || WalletRoutes.PortfolioAssets)
    } else {
      setHasIncorrectPassword(true)
    }
  }, [attemptUnlockWallet, password, history])

  const handleKeyDown = React.useCallback(
    async (event: React.KeyboardEvent<HTMLInputElement>) => {
      if (event.key === 'Enter' && !disabled) {
        await unlockWallet()
      }
    },
    [unlockWallet, disabled]
  )

  const handlePasswordChanged = React.useCallback(
    (value: string) => {
      setPassword(value)

      // clear error
      if (hasIncorrectPassword) {
        setHasIncorrectPassword(false)
      }
    },
    [hasIncorrectPassword]
  )

  const onShowRestore = React.useCallback(() => {
    if (isPanel) {
      openWalletRouteTab(WalletRoutes.Restore)
    } else {
      history.push(WalletRoutes.Restore)
    }
  }, [history, isPanel])

  const onDoubleTap = (e: React.MouseEvent<HTMLButtonElement>) => {
    // Event detail is two for double clicks.
    if (e.detail === 2) {
      getWalletAPIProxy().pageHandler?.unlockWalletUI()
    }
  }

  const isAndroid = loadTimeData.getBoolean('isAndroid') || false

  // render
  if (isAndroid) {
    return (
      <AndroidLockScreenWrapper onClick={onDoubleTap}>
        <DoubleTapIcon />
        <Text
          textSize='22px'
          textColor='primary'
        >
          {getLocale('braveWalletDoubleTapScreen')}
        </Text>
        <Text
          textSize='16px'
          textColor='tertiary'
        >
          {getLocale('braveWalletUnlockAndroidDescription')}
        </Text>
      </AndroidLockScreenWrapper>
    )
  }

  return (
    <StyledWrapper>
      {isPanel && <BraveLogo />}
      <PageIcon />
      <Title>{getLocale('braveWalletUnlockWallet')}</Title>
      <Description>{getLocale('braveWalletLockScreenTitle')}</Description>
      <InputColumn fullWidth={true}>
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
          placeholder={getLocale('braveWalletEnterYourPassword')}
          onChange={handlePasswordChanged}
          onKeyDown={handleKeyDown}
          error={getLocale('braveWalletLockScreenError')}
          hasError={hasIncorrectPassword}
          autoFocus={true}
          value={password}
        />
        <VerticalSpace space='24px' />
        <UnlockButton
          onClick={unlockWallet}
          isDisabled={disabled}
          kind='filled'
          size={isPanel ? 'medium' : 'large'}
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
