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
import { getLocale, splitStringForTag } from '../../../../../common/locale'

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

  const walletAlertText = getLocale('braveWalletCryptoWalletsDescriptionTwo')
  const { beforeTag, duringTag, afterTag } = splitStringForTag(walletAlertText)

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
            <ImportButtonText>{getLocale('braveWalletImportTitle').replace('$1', getLocale('braveWalletImportMetaMaskTitle'))}</ImportButtonText>
          </ImportButton>
        </>
      }
      {cryptoWalletsDetected &&
        <CryptoWalletsAlertBox>
          <CryptoWalletsAlertTitle>{getLocale('braveWalletCryptoWalletsDetected')}</CryptoWalletsAlertTitle>
          <CryptoWalletsAlertDescription>{getLocale('braveWalletCryptoWalletsDescriptionOne')}</CryptoWalletsAlertDescription>
          <CryptoWalletsAlertDescription>
            {beforeTag}
            <SettingsButton onClick={onClickSettings}>{duringTag}</SettingsButton>
            {afterTag}
          </CryptoWalletsAlertDescription>
        </CryptoWalletsAlertBox>
      }
    </StyledWrapper>
  )
}

export default OnboardingWelcome
