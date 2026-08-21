// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch, useHistory } from 'react-router'
import { useSelector } from 'react-redux'

// Types
import { WalletRoutes } from '../../constants/types'
import {
  AccountsTabState, //
} from '../reducers/accounts-tab-reducer'

// Hooks
import {
  usePortfolioVisibleNetworks, //
} from '../../common/hooks/use_portfolio_networks'
import {
  usePortfolioAccounts, //
} from '../../common/hooks/use_portfolio_accounts'

// Styles
import { DefaultPageWrapper } from '../../components/shared/style'

// Components
import { WalletPageLayout } from '../../components/desktop/wallet-page-layout'
import { ExploreWeb3Header } from '../../components/desktop/card-headers/explorer_web3_header'
import {
  EditVisibleAssetsModal, //
} from '../../components/desktop/popup-modals/edit-visible-assets-modal/index'
import { AddAccountModal } from '../../components/desktop/popup-modals/add-account-modal/add-account-modal'
import {
  RemoveAccountModal, //
} from '../../components/desktop/popup-modals/confirm-password-modal/remove-account-modal'
import { AccountSettingsModal } from '../../components/desktop/popup-modals/account-settings-modal/account-settings-modal'
import {
  WalletPageWrapper, //
} from '../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import { NftCollection } from '../screens/nfts/nft_collection'
import { Banners } from '../../components/desktop/banners/banners'
import {
  BackupWalletRoutes, //
} from '../screens/backup-wallet/backup-wallet.routes'
import { Deposit } from '../screens/deposit/deposit'
import { Buy } from '../screens/buy/buy'
import {
  OnboardingSuccess, //
} from '../screens/onboarding/onboarding_success/onboarding_success'
import { PageNotFound } from '../screens/page_not_found/page_not_found'
import { PortfolioOverview } from '../screens/portfolio_overview/portfolio_overview'
import { FungibleAssetDetails } from '../screens/fungible_asset_details/fungible_asset_details'
import { NFTAssetDetails } from '../screens/nft_asset_details/nft_asset_details'
import { MarketView } from '../screens/market/market'
import { MarketAssetDetails } from '../screens/market/market_asset_details'
import { AccountsOverview } from '../screens/accounts_overview/accounts_overview'
import { AccountDetails } from '../screens/account_details/account_details'
import { Swap } from '../screens/swap/swap'
import { SendScreen } from '../screens/send/send_screen/send_screen'

export const UnlockedWalletRoutes = () => {
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
  const hideVisibleAssetsModal = React.useCallback(() => {
    history.push(WalletRoutes.PortfolioAssets)
  }, [history])

  // render
  return (
    <>
      <Switch>
        <Route
          path={WalletRoutes.OnboardingComplete}
          exact
        >
          <WalletPageLayout>
            <OnboardingSuccess />
          </WalletPageLayout>
        </Route>

        <Route path={WalletRoutes.Backup}>
          <WalletPageLayout>
            <BackupWalletRoutes />
          </WalletPageLayout>
        </Route>

        <Route path={WalletRoutes.BuyPageStart}>
          <Buy />
        </Route>

        <Route path={WalletRoutes.DepositPageStart}>
          <Deposit />
        </Route>

        <Route
          path={WalletRoutes.Swap}
          exact={true}
        >
          <Swap key='swap' />
        </Route>

        <Route
          path={WalletRoutes.Bridge}
          exact={true}
        >
          <Swap key='bridge' />
        </Route>

        <Route
          path={WalletRoutes.Send}
          exact={true}
        >
          <SendScreen key='send' />
        </Route>

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
          <NFTAssetDetails />
        </Route>

        <Route
          path={WalletRoutes.PortfolioAsset}
          exact
        >
          <FungibleAssetDetails />
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
              <AccountsOverview />
            </DefaultPageWrapper>
          </WalletPageWrapper>
        </Route>

        <Route path={WalletRoutes.Account}>
          <AccountDetails />
        </Route>

        <Route path={WalletRoutes.Accounts}>
          <AccountsOverview />
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
              <MarketAssetDetails />
            </DefaultPageWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.Explore}
          exact={true}
        >
          <Redirect to={WalletRoutes.Market} />
        </Route>

        {/* Deprecated routes, kept for redirecting to the new routes */}
        <Route path={WalletRoutes.BuyPageDeprecated}>
          <Redirect to={WalletRoutes.BuyPageStart} />
        </Route>

        <Route path={WalletRoutes.DepositPageDeprecated}>
          <Redirect to={WalletRoutes.DepositPageStart} />
        </Route>

        <Route path='*'>
          <PageNotFound />
        </Route>
      </Switch>

      {/* Overlay modals — separate Switch so background routes still render */}
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
