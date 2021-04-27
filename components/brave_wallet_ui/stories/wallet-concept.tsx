import * as React from 'react'
import { WalletWidgetStandIn } from './style'
import {
  SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView
} from '../components/desktop'
import {
  NavTypes,
  NavObjectType
} from '../constants/types'
import { LinkedAccountsOptions, NavOptions, StaticOptions } from '../options/side-nav-options'
import BuySendSwap from '../components/buy-send-swap'
export default {
  title: 'Wallet/Desktop'
}

export const _DesktopWalletConcept = () => {
  const [view, setView] = React.useState<NavTypes>('crypto')
  const [linkedAccounts] = React.useState<NavObjectType[]>(LinkedAccountsOptions)

  // In the future these will be actual paths
  // for example wallet/rewards
  const navigateTo = (path: NavTypes) => {
    setView(path)
  }

  return (
    <WalletPageLayout>
      <SideNav
        navList={NavOptions}
        staticList={StaticOptions}
        selectedButton={view}
        onSubmit={navigateTo}
        linkedAccountsList={linkedAccounts}
      />
      <WalletSubViewLayout>
        {view === 'crypto' ? (
          <CryptoView />
        ) : (
          <div style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
            <h2>{view} view</h2>
          </div>
        )}

      </WalletSubViewLayout>
      <WalletWidgetStandIn>
        <BuySendSwap />
      </WalletWidgetStandIn>
    </WalletPageLayout>
  )
}

_DesktopWalletConcept.story = {
  name: 'Concept'
}
