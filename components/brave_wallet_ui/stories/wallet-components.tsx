import * as React from 'react'

import { DesktopComponentWrapper, DesktopComponentWrapperRow } from './style'
import { SideNav, TopTabNav, ChartControlBar } from '../components/desktop'
import { NavTypes, TopTabNavTypes, BraveWallet } from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import { TopNavOptions } from '../options/top-nav-options'
import { ChartTimelineOptions } from '../options/chart-timeline-options'
import './locale'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { LoadingSkeleton } from '../components/shared'

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
