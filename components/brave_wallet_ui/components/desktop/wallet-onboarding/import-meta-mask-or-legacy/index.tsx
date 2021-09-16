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
  LostButton
} from './style'
import { PasswordInput } from '../../../shared'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onClickLost: () => void
  hasImportError: boolean
  disabled: boolean
  onboardingStep: WalletOnboardingSteps
}

function OnboardingImportMetaMaskOrLegacy (props: Props) {
  const {
    onSubmit,
    onPasswordChanged,
    onClickLost,
    hasImportError,
    onboardingStep,
    disabled
  } = props

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  const isMetaMask = onboardingStep === WalletOnboardingSteps.OnboardingImportMetaMask

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
      <InputColumn>
        <PasswordInput
          placeholder={isMetaMask ? locale.importMetaMaskInput : locale.importBraveLegacyInput}
          onChange={onPasswordChanged}
          error={locale.lockScreenError}
          onKeyDown={handleKeyDown}
          hasError={hasImportError}
          autoFocus={true}
        />
      </InputColumn>
      <NavButton buttonType='primary' text={locale.addAccountImport} onSubmit={onSubmit} disabled={disabled} />
      {!isMetaMask &&
        <LostButton onClick={onClickLost}>{locale.importBraveLegacyAltButton}</LostButton>
      }
    </StyledWrapper>
  )
}

export default OnboardingImportMetaMaskOrLegacy
