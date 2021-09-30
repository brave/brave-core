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
import { getLocale } from '../../../../../common/locale'

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
      <Title>{getLocale('braveWalletWelcomeTitle')}</Title>
      <Description>{getLocale('braveWalletWelcomeDescription')}</Description>
      <NavButton buttonType='primary' text={getLocale('braveWalletWelcomeButton')} onSubmit={onSetup} />
      <RestoreButton onClick={onRestore}>{getLocale('braveWalletWelcomeRestoreButton')}</RestoreButton>
      {metaMaskWalletDetected &&
        <>
          <Divider />
          <ImportButton onClick={onClickImportMetaMask}>
            <MetaMaskIcon />
            <ImportButtonText>{getLocale('braveWalletImportMetaMaskTitle')}</ImportButtonText>
          </ImportButton>
        </>
      }
    </StyledWrapper>
  )
}

export default OnboardingWelcome
