// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, useHistory, Switch, Redirect } from 'react-router-dom'
import { useSelector } from 'react-redux'

// types
import { WalletRoutes } from '../../../../constants/types'
import {
  AccountsTabState, //
} from '../../../../page/reducers/accounts-tab-reducer'

// hooks
import {
  usePortfolioVisibleNetworks, //
} from '../../../../common/hooks/use_portfolio_networks'
import {
  usePortfolioAccounts, //
} from '../../../../common/hooks/use_portfolio_accounts'

// style
import { DefaultPageWrapper } from '../../../shared/style'

// components
import { ExploreWeb3Header } from '../../card-headers/explorer_web3_header'
import {
  EditVisibleAssetsModal, //
} from '../../popup-modals/edit-visible-assets-modal/index'
import { PortfolioOverview } from '../portfolio/portfolio-overview'
import { PortfolioFungibleAsset } from '../portfolio/portfolio-fungible-asset'
import { PortfolioNftAsset } from '../portfolio/portfolio-nft-asset'
import { MarketView } from '../market'
import { Accounts } from '../accounts/accounts'
import { Account } from '../accounts/account'
import { AddAccountModal } from '../../popup-modals/add-account-modal/add-account-modal'
import {
  RemoveAccountModal, //
} from '../../popup-modals/confirm-password-modal/remove-account-modal'
import { AccountSettingsModal } from '../../popup-modals/account-settings-modal/account-settings-modal'
import {
  WalletPageWrapper, //
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import { MarketAsset } from '../market/market_asset'
import { NftCollection } from '../nfts/components/nft_collection'
import {
  PageNotFound, //
} from '../../../../page/screens/page_not_found/page_not_found'
import { Banners } from '../banners/banners'

export interface Props {
  sessionRoute: string | undefined
}

export const CryptoView = ({ sessionRoute }: Props) => {
  // Selectors
  const { accountToRemove, showAccountModal, selectedAccount } = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab,
  )

  // custom hooks
  const { visiblePortfolioNetworks } = usePortfolioVisibleNetworks()
  const { usersFilteredAccounts } = usePortfolioAccounts()

  // routing
  const history = useHistory()

  // methods
  const onShowVisibleAssetsModal = React.useCallback(
    (showModal: boolean) => {
      if (showModal) {
        history.push(WalletRoutes.AddAssetModal)
        return
      }
      history.push(WalletRoutes.PortfolioAssets)
    },
    [history],
  )

  const hideVisibleAssetsModal = React.useCallback(
    () => onShowVisibleAssetsModal(false),
    [onShowVisibleAssetsModal],
  )

  // render
  return (
    <>
      <Switch>
        {/* Portfolio */}
        <Route
          path={WalletRoutes.Portfolio}
          exact={true}
          render={() => <Redirect to={WalletRoutes.PortfolioAssets} />}
        />

        <Route
          path={WalletRoutes.PortfolioAssets}
          exact
        >
          <PortfolioOverview />
        </Route>

        <Route
          path={WalletRoutes.PortfolioNFTs}
          exact
        >
          <PortfolioOverview />
        </Route>

        <Route
          path={WalletRoutes.PortfolioActivity}
          exact
        >
          <PortfolioOverview />
        </Route>

        <Route path={WalletRoutes.AddAssetModal}>
          <PortfolioOverview />
        </Route>

        <Route
          path={WalletRoutes.PortfolioNFTAsset}
          exact
        >
          <PortfolioNftAsset />
        </Route>

        <Route
          path={WalletRoutes.PortfolioAsset}
          exact
        >
          <PortfolioFungibleAsset />
        </Route>

        <Route
          path={WalletRoutes.PortfolioNFTCollection}
          exact
        >
          <NftCollection
            networks={visiblePortfolioNetworks}
            accounts={usersFilteredAccounts}
          />
        </Route>

        {/* Accounts */}
        <Route path={WalletRoutes.AddAccountModal}>
          {/* Show accounts overview in background */}
          <WalletPageWrapper wrapContentInBox={true}>
            <DefaultPageWrapper>
              <Banners />
              <Accounts />
            </DefaultPageWrapper>
          </WalletPageWrapper>
        </Route>

        <Route path={WalletRoutes.Account}>
          <Account />
        </Route>

        <Route path={WalletRoutes.Accounts}>
          <Accounts />
        </Route>

        {/* Market */}
        <Route
          path={WalletRoutes.Market}
          exact={true}
        >
          <WalletPageWrapper
            wrapContentInBox
            cardHeader={<ExploreWeb3Header />}
            useCardInPanel={true}
          >
            <DefaultPageWrapper>
              <MarketView />
            </DefaultPageWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.MarketSub}
          exact={true}
        >
          <WalletPageWrapper wrapContentInBox={true}>
            <DefaultPageWrapper>
              <MarketAsset />
            </DefaultPageWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.Explore}
          exact={true}
        >
          <Redirect to={WalletRoutes.Market} />
        </Route>
        <Route path='*'>
          <PageNotFound />
        </Route>
        <Redirect to={sessionRoute ?? WalletRoutes.PortfolioAssets} />
      </Switch>

      {/* modals */}
      <Switch>
        <Route
          path={WalletRoutes.AddAssetModal}
          exact
        >
          <EditVisibleAssetsModal onClose={hideVisibleAssetsModal} />
        </Route>

        <Route path={WalletRoutes.AddAccountModal}>
          <AddAccountModal />
        </Route>
      </Switch>

      {accountToRemove !== undefined && <RemoveAccountModal />}

      {showAccountModal && selectedAccount && <AccountSettingsModal />}
    </>
  )
}

export default CryptoView
