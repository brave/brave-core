import * as React from 'react'
import {
  WalletOnboardingSteps,
  ImportWalletError
} from '../../../../constants/types'
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
import { getLocale } from '../../../../../common/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onConfirmPasswordChanged: (value: string) => void
  onImportPasswordChanged: (value: string) => void
  onClickLost: () => void
  onUseSamePassword: (selected: boolean) => void
  password: string
  confirmedPassword: string
  useSamePassword: boolean
  importError: ImportWalletError
  hasPasswordError: boolean
  hasConfirmPasswordError: boolean
  disabled: boolean
  onboardingStep: WalletOnboardingSteps
  needsNewPassword: boolean
  useSamePasswordVerified: boolean
  importPassword: string
}

function OnboardingImportMetaMaskOrLegacy (props: Props) {
  const {
    onSubmit,
    onPasswordChanged,
    onConfirmPasswordChanged,
    onImportPasswordChanged,
    onClickLost,
    onUseSamePassword,
    useSamePasswordVerified,
    importPassword,
    password,
    confirmedPassword,
    useSamePassword,
    hasPasswordError,
    hasConfirmPasswordError,
    importError,
    onboardingStep,
    disabled,
    needsNewPassword
  } = props

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  const isMetaMask = onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask

  const onSelectUseSamePassword = (key: string, selected: boolean) => {
    if (key === 'useSamePassword') {
      onUseSamePassword(selected)
    }
  }

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
          onChange={onImportPasswordChanged}
          error={importError.errorMessage ? importError.errorMessage : ''}
          hasError={importError.hasError}
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
              onChange={onConfirmPasswordChanged}
              onKeyDown={handleKeyDown}
              error={getLocale('braveWalletConfirmPasswordError')}
              hasError={hasConfirmPasswordError}
            />
          </InputColumn>
        </>
      }
      <NavButton buttonType='primary' text={getLocale('braveWalletAddAccountImport')} onSubmit={onSubmit} disabled={disabled} />
      {!isMetaMask &&
        <LostButton onClick={onClickLost}>{getLocale('braveWalletImportBraveLegacyAltButton')}</LostButton>
      }
    </StyledWrapper>
  )
}

export default OnboardingImportMetaMaskOrLegacy
