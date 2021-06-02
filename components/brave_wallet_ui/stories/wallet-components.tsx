import * as React from 'react'
import { DesktopComponentWrapper, DesktopComponentWrapperRow, WalletWidgetStandIn } from './style'
import { SideNav, TopTabNav, ChartControlBar, WalletPageLayout, WalletSubViewLayout } from '../components/desktop'
import { NavTypes, TopTabNavTypes, ChartTimelineType } from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import { TopNavOptions } from '../options/top-nav-options'
import { ChartTimelineOptions } from '../options/chart-timeline-options'
import BuySendSwap from '../components/buy-send-swap'
import Onboarding from './screens/onboarding'
import { recoveryPhrase } from './mock-data/user-accounts'

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
        navList={NavOptions}
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

  const navigateTo = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  return (
    <DesktopComponentWrapperRow>
      <TopTabNav
        tabList={TopNavOptions}
        selectedTab={selectedTab}
        onSubmit={navigateTo}
      />
    </DesktopComponentWrapperRow>
  )
}

_DesktopTopTabNav.story = {
  name: 'Top Tab Nav'
}

export const _LineChartControls = () => {
  const [selectedTimeline, setSelectedTimeline] = React.useState<ChartTimelineType>('24HRS')

  const changeTimline = (path: ChartTimelineType) => {
    setSelectedTimeline(path)
  }
  return (
    <DesktopComponentWrapper>
      <ChartControlBar
        onSubmit={changeTimline}
        selectedTimeline={selectedTimeline}
        timelineOptions={ChartTimelineOptions}
      />
    </DesktopComponentWrapper>
  )
}

_LineChartControls.story = {
  name: 'Chart Controls'
}

export const _BuySendSwap = () => {
  return (
    <WalletWidgetStandIn>
      <BuySendSwap />
    </WalletWidgetStandIn>
  )
}

_BuySendSwap.story = {
  name: 'Buy/Send/Swap'
}

export const _Onboarding = () => {

  const complete = () => {
    alert('Wallet Setup Complete!!!')
  }

  const passwordProvided = (password: string) => {
    console.log('Password provided')
  }

  return (
    <WalletPageLayout>
      <WalletSubViewLayout>
        <Onboarding recoveryPhrase={recoveryPhrase} onSubmit={complete} onPasswordProvided={passwordProvided} />
      </WalletSubViewLayout>
    </WalletPageLayout>
  )
}

_Onboarding.story = {
  name: 'Onboarding'
}
