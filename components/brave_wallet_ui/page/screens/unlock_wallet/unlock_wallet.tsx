// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

import Button from '@brave/leo/react/button'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'
import { WalletRoutes } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { openWalletRouteTab } from '../../../utils/routes-utils'
import { UISelectors } from '../../../common/selectors'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { useUnlockWalletMutation } from '../../../common/slices/api.slice'
import getWalletAPIProxy from '../../../common/async/bridge'

// Components
import {
  PasswordInput, //
} from '../../../components/shared/password-input/password-input-v2'

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
  BraveLogo,
} from './unlock_wallet.style'
import { VerticalSpace, Row, Text } from '../../../components/shared/style'

export const UnlockWallet = () => {
  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)

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
        LOCAL_STORAGE_KEYS.SAVED_SESSION_ROUTE,
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
    [unlockWallet, disabled],
  )

  const handlePasswordChanged = React.useCallback(
    (value: string) => {
      setPassword(value)

      // clear error
      if (hasIncorrectPassword) {
        setHasIncorrectPassword(false)
      }
    },
    [hasIncorrectPassword],
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

  // render
  if (isMobile) {
    return (
      <AndroidLockScreenWrapper onClick={onDoubleTap}>
        <DoubleTapIcon />
        <Text
          textSize='22px'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_DOUBLE_TAP_SCREEN)}
        </Text>
        <Text
          textSize='16px'
          textColor='tertiary'
        >
          {getLocale(S.BRAVE_WALLET_UNLOCK_ANDROID_DESCRIPTION)}
        </Text>
      </AndroidLockScreenWrapper>
    )
  }

  return (
    <StyledWrapper>
      {isPanel && <BraveLogo />}
      <PageIcon />
      <Title>{getLocale(S.BRAVE_WALLET_UNLOCK_WALLET)}</Title>
      <Description>{getLocale(S.BRAVE_WALLET_LOCK_SCREEN_TITLE)}</Description>
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
            {getLocale(S.BRAVE_WALLET_INPUT_LABEL_PASSWORD)}
          </InputLabel>
        </Row>
        <PasswordInput
          placeholder={getLocale(S.BRAVE_WALLET_ENTER_YOUR_PASSWORD)}
          onChange={handlePasswordChanged}
          onKeyDown={handleKeyDown}
          error={getLocale(S.BRAVE_WALLET_LOCK_SCREEN_ERROR)}
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
          {getLocale(S.BRAVE_WALLET_LOCK_SCREEN_BUTTON)}
        </UnlockButton>
        <Button
          onClick={onShowRestore}
          kind='plain'
        >
          {getLocale(S.BRAVE_WALLET_WELCOME_RESTORE_BUTTON)}
        </Button>
      </InputColumn>
    </StyledWrapper>
  )
}
