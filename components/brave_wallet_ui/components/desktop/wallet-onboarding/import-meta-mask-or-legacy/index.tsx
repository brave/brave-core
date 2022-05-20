// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory, useLocation } from 'react-router'

// actions
import { WalletPageActions } from '../../../../page/actions'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import {
  PageState,
  WalletRoutes
} from '../../../../constants/types'

// components
import {
  StyledWrapper,
  Title,
  InputColumn,
  Description,
  MetaMaskIcon,
  BraveIcon,
  LostButton,
  PasswordTitle,
  CheckboxRow
} from './style'
import { Checkbox } from 'brave-ui'
import { PasswordInput } from '../../../shared'
import { NavButton } from '../../../extension'

// hooks
import { usePasswordStrength } from '../../../../common/hooks/use-password-strength'

function OnboardingImportMetaMaskOrLegacy () {
  // routing
  const history = useHistory()
  const { pathname: currentRoute } = useLocation()

  // redux
  const dispatch = useDispatch()
  const importWalletError = useSelector(({ page }: { page: PageState }) => page.importWalletError)

  // state
  const [needsNewPassword, setNeedsNewPassword] = React.useState<boolean>(false)
  const [useSamePassword, setUseSamePassword] = React.useState<boolean>(false)
  const [useSamePasswordVerified, setUseSamePasswordVerified] = React.useState<boolean>(false)
  const [importPassword, setImportPassword] = React.useState<string>('')
  const [isStrongImportPassword, setIsStrongImportPassword] = React.useState<boolean>(false)

  // custom hooks
  const {
    checkIsStrongPassword,
    confirmedPassword,
    hasConfirmedPasswordError,
    hasPasswordError,
    onPasswordChanged,
    password,
    setConfirmedPassword
  } = usePasswordStrength()

  // memos
  React.useMemo(() => {
    if (importWalletError.hasError) {
      setUseSamePassword(false)
      setNeedsNewPassword(false)
      setUseSamePasswordVerified(false)
    }
  }, [importWalletError])

  // computed
  const isMetaMask = currentRoute === WalletRoutes.OnboardingImportMetaMask

  const isCreateWalletDisabled = hasConfirmedPasswordError ||
    hasPasswordError ||
    password === '' ||
    confirmedPassword === ''

  const isImportDisabled = isCreateWalletDisabled || importPassword === ''

  // methods
  const setImportWalletError = React.useCallback((hasError: boolean) => {
    dispatch(WalletPageActions.setImportWalletError({ hasError }))
  }, [])

  const handleImportPasswordChanged = React.useCallback(async (value: string) => {
    if (importWalletError.hasError) {
      setImportWalletError(false)
    }

    if (needsNewPassword || useSamePasswordVerified) {
      setNeedsNewPassword(false)
      setUseSamePassword(false)
    }

    setImportPassword(value)

    const isStrong = await checkIsStrongPassword(value)
    setIsStrongImportPassword(isStrong)
  }, [
    importWalletError,
    needsNewPassword,
    useSamePasswordVerified,
    setImportWalletError,
    checkIsStrongPassword,
    setIsStrongImportPassword
  ])

  const onImportMetaMask = React.useCallback((password: string, newPassword: string) => {
    dispatch(WalletPageActions.importFromMetaMask({ password, newPassword }))
  }, [])

  const onImportCryptoWallets = React.useCallback((password: string, newPassword: string) => {
    dispatch(WalletPageActions.importFromCryptoWallets({ password, newPassword }))
  }, [])

  const onImport = React.useCallback(
    () => {
      if (currentRoute === WalletRoutes.OnboardingImportMetaMask) {
        onImportMetaMask(importPassword, confirmedPassword)
      } else {
        onImportCryptoWallets(importPassword, confirmedPassword)
      }
    },
    [
      currentRoute,
      importPassword,
      confirmedPassword,
      onImportMetaMask,
      onImportCryptoWallets
    ]
  )

  const onClickLost = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingCreatePassword)
  }, [])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !isImportDisabled) {
      onImport()
    }
  }, [isImportDisabled, onImport])

  const onSelectUseSamePassword = React.useCallback((key: string, selected: boolean) => {
    if (key === 'useSamePassword') {
      setUseSamePassword(selected)
    }
  }, [])

  // effects
  React.useEffect(() => {
    if (useSamePassword) {
      onPasswordChanged(importPassword)
      setConfirmedPassword(importPassword)
      if (!isStrongImportPassword) {
        setNeedsNewPassword(true)
        setUseSamePasswordVerified(false)
      } else {
        setNeedsNewPassword(false)
        setUseSamePasswordVerified(true)
      }
    } else {
      onPasswordChanged('')
      setConfirmedPassword('')
      setNeedsNewPassword(false)
      setUseSamePasswordVerified(false)
    }
  }, [useSamePassword])

  // render
  return (
    <StyledWrapper>

      {isMetaMask ? (
        <MetaMaskIcon />
      ) : (
        <BraveIcon />
      )}

      <Title>
        {getLocale('braveWalletImportTitle').replace('$1',
          isMetaMask
            ? getLocale('braveWalletImportMetaMaskTitle')
            : getLocale('braveWalletImportBraveLegacyTitle')
        )}
      </Title>

      <Description>
        {getLocale('braveWalletImportDescription').replace('$1',
          isMetaMask
            ? getLocale('braveWalletImportMetaMaskTitle')
            : getLocale('braveWalletImportBraveLegacyTitle')
        )}
      </Description>

      <InputColumn useSamePasswordVerified={useSamePasswordVerified}>
        <PasswordInput
          placeholder={isMetaMask ? getLocale('braveWalletImportMetaMaskInput') : getLocale('braveWalletImportBraveLegacyInput')}
          onChange={handleImportPasswordChanged}
          error={importWalletError.errorMessage ? importWalletError.errorMessage : ''}
          hasError={importWalletError.hasError}
          autoFocus={true}
        />
      </InputColumn>

      {!useSamePasswordVerified &&
        <PasswordTitle needsNewPassword={needsNewPassword}>
          {needsNewPassword
            ? getLocale('braveWalletImportFromExternalNewPassword')
            : getLocale('braveWalletImportFromExternalCreatePassword')}
        </PasswordTitle>
      }

      {!needsNewPassword &&
        <CheckboxRow>
          <Checkbox disabled={importPassword === ''} value={{ useSamePassword: useSamePassword }} onChange={onSelectUseSamePassword}>
            <div data-key='useSamePassword'>{getLocale('braveWalletImportFromExternalPasswordCheck')}</div>
          </Checkbox>
        </CheckboxRow>
      }

      {!useSamePasswordVerified &&
        <>
          <Description>{getLocale('braveWalletCreatePasswordDescription')}</Description>
          <InputColumn>
            <PasswordInput
              placeholder={getLocale('braveWalletCreatePasswordInput')}
              value={password}
              onChange={onPasswordChanged}
              onKeyDown={handleKeyDown}
              error={getLocale('braveWalletCreatePasswordError')}
              hasError={hasPasswordError}
            />
            <PasswordInput
              placeholder={getLocale('braveWalletConfirmPasswordInput')}
              value={confirmedPassword}
              onChange={setConfirmedPassword}
              onKeyDown={handleKeyDown}
              error={getLocale('braveWalletConfirmPasswordError')}
              hasError={hasConfirmedPasswordError}
            />
          </InputColumn>
        </>
      }

      <NavButton
        buttonType='primary'
        text={getLocale('braveWalletAddAccountImport')}
        onSubmit={onImport}
        disabled={isImportDisabled}
      />

      {!isMetaMask &&
        <LostButton onClick={onClickLost}>
          {getLocale('braveWalletImportBraveLegacyAltButton')}
        </LostButton>
      }
    </StyledWrapper>
  )
}

export default OnboardingImportMetaMaskOrLegacy
