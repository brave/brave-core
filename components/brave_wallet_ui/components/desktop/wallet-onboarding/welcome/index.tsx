import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  RestoreButton,
  Divider,
  ImportButton,
  SettingsButton,
  ImportButtonText,
  MetaMaskIcon,
  CryptoWalletsAlertBox,
  CryptoWalletsAlertTitle,
  CryptoWalletsAlertDescription
} from './style'
import { NavButton } from '../../../extension'
import { getLocale } from '../../../../../common/locale'

export interface Props {
  onSetup: () => void
  onRestore: () => void
  onClickImportMetaMask: () => void
  metaMaskWalletDetected: boolean
  cryptoWalletsDetected: boolean
}

function OnboardingWelcome (props: Props) {
  const { onRestore, onSetup, onClickImportMetaMask, metaMaskWalletDetected, cryptoWalletsDetected } = props

  const onClickSettings = () => {
    chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

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
            <ImportButtonText>{getLocale('braveWalletImportTitle')} {getLocale('braveWalletImportMetaMaskTitle')}</ImportButtonText>
          </ImportButton>
        </>
      }
      {cryptoWalletsDetected &&
        <CryptoWalletsAlertBox>
          <CryptoWalletsAlertTitle>{getLocale('braveWalletCryptoWalletsDetected')}</CryptoWalletsAlertTitle>
          <CryptoWalletsAlertDescription>{getLocale('braveWalletCryptoWalletsDescriptionOne')}</CryptoWalletsAlertDescription>
          <CryptoWalletsAlertDescription>
            {getLocale('braveWalletCryptoWalletsDescriptionTwoFirst')}{` `}
            <SettingsButton onClick={onClickSettings}>{getLocale('braveWalletWalletPopupSettings')}</SettingsButton>{` `}
            {getLocale('braveWalletCryptoWalletsDescriptionTwoSecond')}
          </CryptoWalletsAlertDescription>
        </CryptoWalletsAlertBox>
      }
    </StyledWrapper>
  )
}

export default OnboardingWelcome
