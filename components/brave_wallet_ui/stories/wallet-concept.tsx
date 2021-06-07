import * as React from 'react'
import { WalletWidgetStandIn } from './style'
import {
  SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView,
  LockScreen
} from '../components/desktop'
import {
  NavTypes
} from '../constants/types'
import Onboarding from './screens/onboarding'
import BackupWallet from './screens/backup-wallet'
import { NavOptions } from '../options/side-nav-options'
import BuySendSwap from '../components/buy-send-swap'
import { recoveryPhrase } from './mock-data/user-accounts'
export default {
  title: 'Wallet/Desktop',
  argTypes: {
    onboarding: { control: { type: 'boolean', onboard: true } },
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const _DesktopWalletConcept = (args: { onboarding: boolean, locked: boolean }) => {
  const { onboarding, locked } = args
  const [view, setView] = React.useState<NavTypes>('crypto')
  const [needsOnboarding, setNeedsOnboarding] = React.useState<boolean>(onboarding)
  const [walletLocked, setWalletLocked] = React.useState<boolean>(locked)
  const [needsBackup, setNeedsBackup] = React.useState<boolean>(true)
  const [showBackup, setShowBackup] = React.useState<boolean>(false)
  const [inputValue, setInputValue] = React.useState<string>('')
  const [hasRestoreError, setHasRestoreError] = React.useState<boolean>(false)
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)

  // In the future these will be actual paths
  // for example wallet/rewards
  const navigateTo = (path: NavTypes) => {
    setView(path)
  }

  const completeWalletSetup = (recoveryVerified: boolean) => {
    setNeedsOnboarding(false)
    setNeedsBackup(recoveryVerified)
  }

  const onWalletBackedUp = () => {
    setNeedsBackup(false)
  }

  const passwordProvided = (password: string) => {
    console.log('Password provided')
  }

  const unlockWallet = () => {
    if (inputValue !== 'password') {
      setHasPasswordError(true)
    } else {
      setWalletLocked(false)
    }
  }

  const lockWallet = () => {
    setWalletLocked(true)
  }

  const handlePasswordChanged = (value: string) => {
    setHasPasswordError(false)
    setInputValue(value)
  }

  const onShowBackup = () => {
    setShowBackup(true)
  }

  const onHideBackup = () => {
    setShowBackup(false)
  }

  const onRestore = (phrase: string, password: string) => {
    if (JSON.stringify(phrase.split(' ')) === JSON.stringify(recoveryPhrase)) {
      completeWalletSetup(true)
    } else {
      setHasRestoreError(true)
    }
  }

  return (
    <WalletPageLayout>
      <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      />
      <WalletSubViewLayout>
        {needsOnboarding ?
          (
            <Onboarding
              recoveryPhrase={recoveryPhrase}
              onSubmit={completeWalletSetup}
              onPasswordProvided={passwordProvided}
              onRestore={onRestore}
              hasRestoreError={hasRestoreError}
            />
          ) : (
            <>
              {view === 'crypto' ? (
                <>
                  {walletLocked ? (
                    <LockScreen hasPasswordError={hasPasswordError} onSubmit={unlockWallet} disabled={inputValue === ''} onPasswordChanged={handlePasswordChanged} />
                  ) : (
                    <>
                      {showBackup ? (
                        <BackupWallet
                          isOnboarding={false}
                          onCancel={onHideBackup}
                          onSubmit={onWalletBackedUp}
                          recoveryPhrase={recoveryPhrase}
                        />
                      ) : (
                        <CryptoView onLockWallet={lockWallet} needsBackup={needsBackup} onShowBackup={onShowBackup} />
                      )}
                    </>
                  )}
                </>
              ) : (
                <div style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
                  <h2>{view} view</h2>
                </div>
              )}
            </>
          )}
      </WalletSubViewLayout>
      <WalletWidgetStandIn>
        {!needsOnboarding && !walletLocked &&
          <BuySendSwap />
        }
      </WalletWidgetStandIn>
    </WalletPageLayout>
  )
}

_DesktopWalletConcept.args = {
  onboarding: true,
  locked: false
}

_DesktopWalletConcept.story = {
  name: 'Concept'
}
