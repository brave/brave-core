// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory, useLocation } from 'react-router'

// Utils
import { getLocale } from '../../../../common/locale'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'

// Components
import { BackButton } from '../../../components/shared/back-button'
import { PasswordInput } from '../../../components/shared/password-input'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import { Checkbox } from 'brave-ui'

// Styles
import {
  StyledWrapper,
  Title,
  Description,
  RecoveryPhraseInput,
  ErrorText,
  CheckboxRow,
  LegacyCheckboxRow,
  FormWrapper,
  InputColumn,
  FormText
} from './restore-wallet.style'

// hooks
import { usePasswordStrength } from '../../../common/hooks/use-password-strength'

import * as WalletPageActions from '../../../page/actions/wallet_page_actions'
import { PageState, WalletRoutes, WalletState } from '../../../constants/types'

export const RestoreWallet = () => {
  // routing
  let history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // redux
  const dispatch = useDispatch()
  const isWalletCreated = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletCreated)
  const isWalletLocked = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletLocked)
  const invalidMnemonic = useSelector(({ page }: { page: PageState }) => page.invalidMnemonic)

  // custom hooks
  const {
    hasConfirmedPasswordError,
    hasPasswordError,
    password,
    onPasswordChanged: handlePasswordChanged,
    setConfirmedPassword: handleConfirmPasswordChanged,
    isValid: isPasswordValid
  } = usePasswordStrength()

  // state
  const [showRecoveryPhrase, setShowRecoveryPhrase] = React.useState<boolean>(false)
  const [isLegacyWallet, setIsLegacyWallet] = React.useState<boolean>(false)
  const [recoveryPhrase, setRecoveryPhrase] = React.useState<string>('')

  // memos
  const isValidRecoveryPhrase = React.useMemo(() => {
    if (recoveryPhrase.trim().split(/\s+/g).length >= 12) {
      return true
    } else {
      return false
    }
  }, [recoveryPhrase])

  // computed
  const isDisabled = !isValidRecoveryPhrase || !isPasswordValid

  // methods
  const toggleShowRestore = React.useCallback(() => {
    if (walletLocation === WalletRoutes.Restore) {
      // If a user has not yet created a wallet and clicks Restore
      // from the panel, we need to route to onboarding if they click back.
      if (!isWalletCreated) {
        history.push(WalletRoutes.Onboarding)
        return
      }
      // If a user has created a wallet and clicks Restore from the panel
      // while the wallet is locked, we need to route to unlock if they click back.
      if (isWalletCreated && isWalletLocked) {
        history.push(WalletRoutes.Unlock)
      }
    } else {
      history.push(WalletRoutes.Restore)
    }
  }, [walletLocation, isWalletCreated, isWalletLocked])

  const onBack = React.useCallback(() => {
    toggleShowRestore()
    setRecoveryPhrase('')
  }, [toggleShowRestore])

  const onSubmitRestore = React.useCallback(async () => {
    dispatch(WalletPageActions.restoreWallet({
      // added an additional trim here in case the phrase length is
      // 12, 15, 18 or 21 long and has a space at the end.
      mnemonic: recoveryPhrase.trimEnd(),
      password,
      isLegacy: isLegacyWallet,
      completeWalletSetup: true
    }))
    history.push(WalletRoutes.Portfolio)
  }, [recoveryPhrase, password, isLegacyWallet])

  const handleRecoveryPhraseChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const value = event.target.value

    // This prevents there from being a space at the begining of the phrase.
    const removeBegginingWhiteSpace = value.trimStart()

    // This Prevents there from being more than one space between words.
    const removedDoubleSpaces = removeBegginingWhiteSpace.replace(/ +(?= )/g, '')

    // Although the above removes double spaces, it is initialy recognized as a
    // a double-space before it is removed and macOS automatically replaces double-spaces with a period.
    const removePeriod = removedDoubleSpaces.replace(/['/.']/g, '')

    // This prevents an extra space at the end of a 24 word phrase.
    if (recoveryPhrase.split(' ').length === 24) {
      setRecoveryPhrase(removePeriod.trimEnd())
    } else {
      setRecoveryPhrase(removePeriod)
    }
  }, [])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !isDisabled) {
      onSubmitRestore()
    }
  }, [onSubmitRestore, isDisabled])

  const onShowRecoveryPhrase = React.useCallback((key: string, selected: boolean) => {
    if (key === 'showPhrase') {
      setShowRecoveryPhrase(selected)
    }
  }, [])

  const onSetIsLegacyWallet = React.useCallback((key: string, selected: boolean) => {
    if (key === 'isLegacy') {
      setIsLegacyWallet(selected)
    }
  }, [])

  const onClearClipboard = React.useCallback(() => {
    copyToClipboard('')
  }, [])

  // effects
  React.useEffect(() => {
    if (invalidMnemonic) {
      setTimeout(
        () => {
          dispatch(WalletPageActions.hasMnemonicError(false))
        },
        5000
      )
    }
  }, [invalidMnemonic])

  // render
  return (
    <>
      <BackButton onSubmit={onBack} />

      <StyledWrapper>

        <Title>{getLocale('braveWalletRestoreTite')}</Title>
        <Description>{getLocale('braveWalletRestoreDescription')}</Description>

        <FormWrapper>
          <RecoveryPhraseInput
            autoFocus={true}
            placeholder={getLocale('braveWalletRestorePlaceholder')}
            onChange={handleRecoveryPhraseChanged}
            value={recoveryPhrase}
            type={showRecoveryPhrase ? 'text' : 'password'}
            autoComplete='off'
            onPaste={onClearClipboard}
          />

          {invalidMnemonic && <ErrorText>{getLocale('braveWalletRestoreError')}</ErrorText>}

          {recoveryPhrase.split(' ').length === 24 &&
            <LegacyCheckboxRow>
              <Checkbox value={{ isLegacy: isLegacyWallet }} onChange={onSetIsLegacyWallet}>
                <div data-key='isLegacy'>{getLocale('braveWalletRestoreLegacyCheckBox')}</div>
              </Checkbox>
            </LegacyCheckboxRow>
          }

          <CheckboxRow>
            <Checkbox value={{ showPhrase: showRecoveryPhrase }} onChange={onShowRecoveryPhrase}>
              <div data-key='showPhrase'>{getLocale('braveWalletRestoreShowPhrase')}</div>
            </Checkbox>
          </CheckboxRow>

          <FormText>{getLocale('braveWalletRestoreFormText')}</FormText>
          <Description textAlign='left'>{getLocale('braveWalletCreatePasswordDescription')}</Description>
          <InputColumn>
            <PasswordInput
              placeholder={getLocale('braveWalletCreatePasswordInput')}
              onChange={handlePasswordChanged}
              hasError={hasPasswordError}
              error={getLocale('braveWalletCreatePasswordError')}
              onKeyDown={handleKeyDown}
            />
            <PasswordInput
              placeholder={getLocale('braveWalletConfirmPasswordInput')}
              onChange={handleConfirmPasswordChanged}
              hasError={hasConfirmedPasswordError}
              error={getLocale('braveWalletConfirmPasswordError')}
              onKeyDown={handleKeyDown}
            />
          </InputColumn>

        </FormWrapper>

        <NavButton
          disabled={isDisabled}
          buttonType='primary'
          text={getLocale('braveWalletWelcomeRestoreButton')}
          onSubmit={onSubmitRestore}
        />

      </StyledWrapper>
    </>
  )
}

export default RestoreWallet
