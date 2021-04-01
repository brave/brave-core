import * as React from 'react'
import { DesktopComponentWrapper } from './style'
import { SideNav } from '../components/desktop'
import { NavTypes, NavObjectType } from '../constants/types'
import { LinkedAccountsOptions, NavOptions, StaticOptions } from '../mock-data/side-nav-options'

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
