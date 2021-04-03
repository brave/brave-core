import * as React from 'react'
import { DesktopComponentWrapper, DesktopComponentWrapperRow } from './style'
import { SideNav, TopTabNav } from '../components/desktop'
import { NavTypes, NavObjectType, TopTabNavTypes } from '../constants/types'
import { LinkedAccountsOptions, NavOptions, StaticOptions } from '../mock-data/side-nav-options'
import { TopNavOptions } from '../mock-data/top-nav-options'

export default {
  title: 'Wallet/Desktop/Components',
  parameters: {
    layout: 'centered'
  }
}

export const _DesktopSideNav = () => {
  const [selectedButton, setSelectedButton] = React.useState<NavTypes>('crypto')
  const [linkedAccounts] = React.useState<NavObjectType[]>([LinkedAccountsOptions[0]])

  const navigateTo = (path: NavTypes) => {
    setSelectedButton(path)
  }

  return (
    <DesktopComponentWrapper>
      <SideNav
        navList={NavOptions}
        staticList={StaticOptions}
        selectedButton={selectedButton}
        onSubmit={navigateTo}
        linkedAccountsList={linkedAccounts}
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
