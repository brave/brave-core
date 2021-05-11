// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'

import * as WalletPageActions from './actions/wallet_page_actions'
import store from './store'

import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'

import { WalletWidgetStandIn } from '../stories/style'
import {
  SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView
} from '../components/desktop'
import {
  NavTypes,
  NavObjectType,
  WalletState,
  PageState,
  WalletPageState
} from '../constants/types'
import { LinkedAccountsOptions, NavOptions, StaticOptions } from '../options/side-nav-options'
import BuySendSwap from '../components/buy-send-swap'

type Props = {
  wallet: WalletState
  page: PageState
  actions: typeof WalletPageActions
}

function Container (props: Props) {
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

function mapStateToProps (state: WalletPageState): Partial<Props> {
  return {
    page: state.page,
    wallet: state.wallet
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    actions: bindActionCreators(WalletPageActions, store.dispatch.bind(store))
  }
}

export default connect(mapStateToProps, mapDispatchToProps)(Container)
