import * as React from 'react'
import { WalletOnboardingSteps } from '../../../../constants/types'
import {
  StyledWrapper,
  Title,
  PageIcons,
  InputColumn,
  Description,
  MetaMaskIcon,
  BraveIcon,
  ArrowIcon,
  LostButton,
  PasswordTitle,
  CheckboxRow
} from './style'
import { Checkbox } from 'brave-ui'
import { PasswordInput } from '../../../shared'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

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
  hasImportError: boolean
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
    hasImportError,
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
      <PageIcons>
        {isMetaMask &&
          <>
            <MetaMaskIcon />
            <ArrowIcon />
          </>
        }
        <BraveIcon isMetaMask={isMetaMask} />
      </PageIcons>
      <Title>
        {isMetaMask ?
          locale.importMetaMaskTitle
          : locale.importBraveLegacyTitle}
      </Title>
      <Description>
        {isMetaMask ?
          locale.importMetaMaskDescription
          : locale.importBraveLegacyDescription
        }
      </Description>
      <InputColumn useSamePasswordVerified={useSamePasswordVerified}>
        <PasswordInput
          placeholder={isMetaMask ? locale.importMetaMaskInput : locale.importBraveLegacyInput}
          onChange={onImportPasswordChanged}
          error={locale.lockScreenError}
          hasError={hasImportError}
          autoFocus={true}
        />
      </InputColumn>
      {!useSamePasswordVerified &&
        <PasswordTitle needsNewPassword={needsNewPassword}>
          {needsNewPassword ?
            locale.braveWalletImportFromExternalNewPassword
            : locale.braveWalletImportFromExternalCreatePassword}
        </PasswordTitle>
      }
      {!needsNewPassword &&
        <CheckboxRow>
          <Checkbox disabled={importPassword === ''} value={{ useSamePassword: useSamePassword }} onChange={onSelectUseSamePassword}>
            <div data-key='useSamePassword'>{locale.braveWalletImportFromExternalPasswordCheck}</div>
          </Checkbox>
        </CheckboxRow>
      }
      {!useSamePasswordVerified &&
        <InputColumn>
          <PasswordInput
            placeholder={locale.createPasswordInput}
            value={password}
            onChange={onPasswordChanged}
            error={locale.createPasswordError}
            hasError={hasPasswordError}
          />
          <PasswordInput
            placeholder={locale.confirmPasswordInput}
            value={confirmedPassword}
            onChange={onConfirmPasswordChanged}
            onKeyDown={handleKeyDown}
            error={locale.confirmPasswordError}
            hasError={hasConfirmPasswordError}
          />
        </InputColumn>
      }
      <NavButton buttonType='primary' text={locale.addAccountImport} onSubmit={onSubmit} disabled={disabled} />
      {!isMetaMask &&
        <LostButton onClick={onClickLost}>{locale.importBraveLegacyAltButton}</LostButton>
      }
    </StyledWrapper>
  )
}

export default OnboardingImportMetaMaskOrLegacy
