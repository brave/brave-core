import * as React from 'react'

import './locale'
import WalletPageStory from './wrappers/wallet-page-story-wrapper'
import Container from '../page/container'

export default {
  title: 'Wallet/Desktop',
  argTypes: {
    onboarding: { control: { type: 'boolean', onboard: false } },
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const _DesktopWalletConcept = () => {
  return (
    <WalletPageStory
      walletStateOverride={{
        hasIncorrectPassword: false,
        isWalletCreated: true,
        isSolanaEnabled: true,
        isFilecoinEnabled: true
      }}
      pageStateOverride={{
        hasInitialized: true
      }}
    >
      <Container />
    </WalletPageStory>
  )
}

_DesktopWalletConcept.story = {
  name: 'Concept'
}

export const _WalletOnboardingConcept = () => {
  return (
    <WalletPageStory
      walletStateOverride={{
        hasIncorrectPassword: false,
        isWalletCreated: false,
        isSolanaEnabled: true,
        isFilecoinEnabled: true,
        isWalletBackedUp: false
      }}
      pageStateOverride={{
        hasInitialized: true,
        mnemonic: undefined,
        isCryptoWalletsInitialized: false,
        isMetaMaskInitialized: true,
        setupStillInProgress: true,
        isImportWalletsCheckComplete: true
      }}
    >
      <Container />
    </WalletPageStory>
  )
}

_WalletOnboardingConcept.story = {
  name: 'Onboarding Concept'
}
