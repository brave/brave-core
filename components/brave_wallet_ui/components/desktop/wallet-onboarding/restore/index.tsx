import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory, useLocation } from 'react-router'

// Components
import { PasswordInput, BackButton } from '../../../shared'
import { NavButton } from '../../../extension'
import { Checkbox } from 'brave-ui'

// Utils
import { getLocale } from '../../../../../common/locale'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'

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
} from './style'

// hooks
import { useLib } from '../../../../common/hooks/useLib'

import * as WalletPageActions from '../../../../page/actions/wallet_page_actions'
import { PageState, WalletRoutes, WalletState } from '../../../../constants/types'

export const OnboardingRestore = () => {
  // routing
  let history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // redux
  const dispatch = useDispatch()
  const {
    isWalletCreated,
    isWalletLocked
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const { invalidMnemonic } = useSelector(({ page }: { page: PageState }) => page)

  // custom hooks
  const { isStrongPassword: checkIsStrongPassword } = useLib()

  // state
  const [showRecoveryPhrase, setShowRecoveryPhrase] = React.useState<boolean>(false)
  const [isLegacyWallet, setIsLegacyWallet] = React.useState<boolean>(false)
  const [isStrongPassword, setIsStrongPassword] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [confirmedPassword, setConfirmedPassword] = React.useState<string>('')
  const [recoveryPhrase, setRecoveryPhrase] = React.useState<string>('')

  // methods
  const onBack = () => {
    toggleShowRestore()
    setRecoveryPhrase('')
  }

  const onSubmitRestore = () => {
    dispatch(WalletPageActions.restoreWallet({
      // added an additional trim here in case the phrase length is
      // 12, 15, 18 or 21 long and has a space at the end.
      mnemonic: recoveryPhrase.trimEnd(),
      password,
      isLegacy: isLegacyWallet
    }))
  }

  const handlePasswordChanged = async (value: string) => {
    setPassword(value)
    const isStrong = await checkIsStrongPassword(value)
    setIsStrongPassword(isStrong)
  }

  const handleConfirmPasswordChanged = (value: string) => {
    setConfirmedPassword(value)
  }

  const handleRecoveryPhraseChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
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
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !isDisabled) {
      onSubmitRestore()
    }
  }

  const onShowRecoveryPhrase = (key: string, selected: boolean) => {
    if (key === 'showPhrase') {
      setShowRecoveryPhrase(selected)
    }
  }

  const onSetIsLegacyWallet = (key: string, selected: boolean) => {
    if (key === 'isLegacy') {
      setIsLegacyWallet(selected)
    }
  }

  const onClearClipboard = () => {
    copyToClipboard('')
  }

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
  }, [walletLocation, isWalletCreated])

  // memos
  const isValidRecoveryPhrase = React.useMemo(() => {
    if (recoveryPhrase.trim().split(/\s+/g).length >= 12) {
      return false
    } else {
      return true
    }
  }, [recoveryPhrase])

  const checkPassword = React.useMemo(() => {
    if (password === '') {
      return false
    }
    return !isStrongPassword
  }, [password, isStrongPassword])

  const checkConfirmedPassword = React.useMemo(() => {
    if (confirmedPassword === '') {
      return false
    } else {
      return confirmedPassword !== password
    }
  }, [confirmedPassword, password])

  // computed
  const isDisabled = isValidRecoveryPhrase || checkConfirmedPassword || checkPassword || password === '' || confirmedPassword === ''

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
              hasError={checkPassword}
              error={getLocale('braveWalletCreatePasswordError')}
              onKeyDown={handleKeyDown}
            />
            <PasswordInput
              placeholder={getLocale('braveWalletConfirmPasswordInput')}
              onChange={handleConfirmPasswordChanged}
              hasError={checkConfirmedPassword}
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

export default OnboardingRestore
