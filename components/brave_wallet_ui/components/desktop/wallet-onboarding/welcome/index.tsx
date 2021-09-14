import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  RestoreButton,
  Divider,
  ImportButton,
  ImportButtonText,
  MetaMaskIcon
} from './style'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

export interface Props {
  onSetup: () => void
  onRestore: () => void
  onClickImportMetaMask: () => void
  metaMaskWalletDetected: boolean
}

function OnboardingWelcome (props: Props) {
  const { onRestore, onSetup, onClickImportMetaMask, metaMaskWalletDetected } = props
  return (
    <StyledWrapper>
      <PageIcon />
      <Title>{locale.welcomeTitle}</Title>
      <Description>{locale.welcomeDescription}</Description>
      <NavButton buttonType='primary' text={locale.welcomeButton} onSubmit={onSetup} />
      <RestoreButton onClick={onRestore}>{locale.welcomeRestoreButton}</RestoreButton>
      {metaMaskWalletDetected &&
        <>
          <Divider />
          <ImportButton onClick={onClickImportMetaMask}>
            <MetaMaskIcon />
            <ImportButtonText>{locale.importMetaMaskTitle}</ImportButtonText>
          </ImportButton>
        </>
      }
    </StyledWrapper>
  )
}

export default OnboardingWelcome
