import * as React from 'react'
import { Provider } from 'react-redux'
import { BrowserRouter } from 'react-router-dom'

import { DesktopComponentWrapper, DesktopComponentWrapperRow } from './style'
import { SideNav, TopTabNav, ChartControlBar, WalletPageLayout, WalletSubViewLayout, OnboardingVerify } from '../components/desktop'
import { NavTypes, TopTabNavTypes, BraveWallet } from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import { TopNavOptions } from '../options/top-nav-options'
import { ChartTimelineOptions } from '../options/chart-timeline-options'
import Onboarding from './screens/onboarding'
import './locale'
import { mockRecoveryPhrase } from './mock-data/user-accounts'
import BackupWallet from './screens/backup-wallet'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { LoadingSkeleton } from '../components/shared'
import { createStore, combineReducers } from 'redux'
import { createSendCryptoReducer } from '../common/reducers/send_crypto_reducer'
import { createWalletReducer } from '../common/reducers/wallet_reducer'
import { createPageReducer } from '../page/reducers/page_reducer'
import { mockPageState } from './mock-data/mock-page-state'
import { mockWalletState } from './mock-data/mock-wallet-state'
import { mockSendCryptoState } from './mock-data/send-crypto-state'
import * as Lib from '../common/async/__mocks__/lib'
import { LibContext } from '../common/context/lib.context'

const store = createStore(combineReducers({
  wallet: createWalletReducer(mockWalletState),
  page: createPageReducer(mockPageState),
  sendCrypto: createSendCryptoReducer(mockSendCryptoState)
}))

export default {
  title: 'Wallet/Desktop/Components',
  parameters: {
    layout: 'centered'
  }
}

export const _DesktopSideNav = () => {
  const [selectedButton, setSelectedButton] = React.useState<NavTypes>('crypto')

  const navigateTo = (path: NavTypes) => {
    setSelectedButton(path)
  }

  return (
    <DesktopComponentWrapper>
      <SideNav
        navList={NavOptions()}
        selectedButton={selectedButton}
        onSubmit={navigateTo}
      />
    </DesktopComponentWrapper>
  )
}

_DesktopSideNav.story = {
  name: 'Side Nav'
}

export const _DesktopTopTabNav = () => {
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')

  const onSelectTab = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  return (
    <DesktopComponentWrapperRow>
      <TopTabNav
        tabList={TopNavOptions()}
        selectedTab={selectedTab}
        onSelectTab={onSelectTab}
      />
    </DesktopComponentWrapperRow>
  )
}

_DesktopTopTabNav.story = {
  name: 'Top Tab Nav'
}

export const _LineChartControls = () => {
  const [selectedTimeline, setSelectedTimeline] = React.useState<BraveWallet.AssetPriceTimeframe>(BraveWallet.AssetPriceTimeframe.OneDay)

  const changeTimline = (path: BraveWallet.AssetPriceTimeframe) => {
    setSelectedTimeline(path)
  }
  return (
    <DesktopComponentWrapper>
      <ChartControlBar
        onSubmit={changeTimline}
        selectedTimeline={selectedTimeline}
        timelineOptions={ChartTimelineOptions()}
      />
    </DesktopComponentWrapper>
  )
}

_LineChartControls.story = {
  name: 'Chart Controls'
}

export const _Onboarding = () => {
  return (
    <Provider store={store}>
      <LibContext.Provider value={Lib as any}>
        <BrowserRouter>
          <WalletPageLayout>
            <WalletSubViewLayout>
              <Onboarding />
            </WalletSubViewLayout>
          </WalletPageLayout>
        </BrowserRouter>
      </LibContext.Provider>
    </Provider>
  )
}

_Onboarding.story = {
  name: 'Onboarding'
}

export const _BackupWallet = () => {
  const complete = () => {
    alert('Wallet Setup Complete!!!')
  }

  return <BackupWallet
    recoveryPhrase={mockRecoveryPhrase}
    onSubmit={complete}
    onCancel={complete}
    isOnboarding={true}
  />
}

_BackupWallet.story = {
  name: 'BackupWallet'
}

export const _OnboardingVerify = () => {
  return <OnboardingVerify
    recoveryPhrase={mockRecoveryPhrase}
    onNextStep={() => console.log('done')}
  />
}

_OnboardingVerify.story = {
  name: 'Onboarding Verify'
}

export const _SweepstakesBanner = () => {
  return <SweepstakesBanner
    startDate={new Date(Date.now())}
    endDate={new Date(Date.now() + 1)}
  />
}

_SweepstakesBanner.story = {
  name: 'Sweepstakes Banner'
}

export const _LoadingSkeleton = () => {
  return (
  <div
    style={{
      width: '600px',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    }}>
    <LoadingSkeleton
      width={500}
      height={20}
      count={5}
    />
  </div>)
}

_LoadingSkeleton.story = {
  name: 'Loading Skeleton'
}
