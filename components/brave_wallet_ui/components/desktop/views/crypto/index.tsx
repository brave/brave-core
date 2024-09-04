// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, useHistory, Switch, Redirect } from 'react-router-dom'
import { useSelector } from 'react-redux'

// utils
import { loadTimeData } from '../../../../../common/loadTimeData'
import { getLocale } from '../../../../../common/locale'
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'
import { openWalletSettings } from '../../../../utils/routes-utils'
import {
  useGetDefaultEthereumWalletQuery,
  useGetDefaultSolanaWalletQuery,
  useGetIsMetaMaskInstalledQuery,
  useGetIsWalletBackedUpQuery //
} from '../../../../common/slices/api.slice'

// types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
import {
  AccountsTabState //
} from '../../../../page/reducers/accounts-tab-reducer'

// hooks
import {
  usePortfolioVisibleNetworks //
} from '../../../../common/hooks/use_portfolio_networks'
import {
  usePortfolioAccounts //
} from '../../../../common/hooks/use_portfolio_accounts'

// style
import { StyledWrapper } from './style'
import { Column } from '../../../shared/style'

// components
import getWalletPageApiProxy from '../../../../page/wallet_page_api_proxy'
import { WalletBanner } from '../../wallet-banner/index'
import { ExploreWeb3Header } from '../../card-headers/explorer_web3_header'
import {
  EditVisibleAssetsModal //
} from '../../popup-modals/edit-visible-assets-modal/index'
import { PortfolioOverview } from '../portfolio/portfolio-overview'
import { PortfolioFungibleAsset } from '../portfolio/portfolio-fungible-asset'
import { PortfolioNftAsset } from '../portfolio/portfolio-nft-asset'
import { MarketView } from '../market'
import { Accounts } from '../accounts/accounts'
import { Account } from '../accounts/account'
import { AddAccountModal } from '../../popup-modals/add-account-modal/add-account-modal'
import {
  RemoveAccountModal //
} from '../../popup-modals/confirm-password-modal/remove-account-modal'
import { AccountSettingsModal } from '../../popup-modals/account-settings-modal/account-settings-modal'
import TransactionsScreen from '../../../../page/screens/transactions/transactions-screen'
import {
  WalletPageWrapper //
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import {
  PortfolioOverviewHeader //
} from '../../card-headers/portfolio-overview-header'
import { MarketAsset } from '../market/market_asset'
import { ExploreWeb3View } from '../explore_web3/explore_web3'
import { NftCollection } from '../nfts/components/nft_collection'

export interface Props {
  sessionRoute: string | undefined
}

export const CryptoView = ({ sessionRoute }: Props) => {
  const isAndroid = loadTimeData.getBoolean('isAndroid') || false

  const isPanel = useSafeUISelector(UISelectors.isPanel)

  const { accountToRemove, showAccountModal, selectedAccount } = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab
  )

  // custom hooks
  const { visiblePortfolioNetworks } = usePortfolioVisibleNetworks()
  const { usersFilteredAccounts } = usePortfolioAccounts()

  // queries
  const {
    data: isMetaMaskInstalled,
    isLoading: isCheckingInstalledExtensions
  } = useGetIsMetaMaskInstalledQuery()
  const {
    data: defaultEthereumWallet,
    isLoading: isLoadingDefaultEthereumWallet
  } = useGetDefaultEthereumWalletQuery()
  const { data: defaultSolanaWallet, isLoading: isLoadingDefaultSolanaWallet } =
    useGetDefaultSolanaWalletQuery()
  const {
    data: isWalletBackedUp = false,
    isLoading: isCheckingWalletBackupStatus
  } = useGetIsWalletBackedUpQuery()

  // state
  const [isBackupWarningDismissed, setDismissBackupWarning] =
    React.useState<boolean>(isWalletBackedUp)
  const [isDefaultWalletBannerDismissed, setDismissDefaultWalletBanner] =
    React.useState<boolean>(false)

  // routing
  const history = useHistory()

  // methods
  const onShowBackup = React.useCallback(() => {
    if (isAndroid) {
      getWalletPageApiProxy().pageHandler.showWalletBackupUI()
      return
    }

    if (isPanel) {
      chrome.tabs.create(
        {
          url: `chrome://wallet${WalletRoutes.Backup}`
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message
            )
          }
        }
      )
      return
    }
    history.push(WalletRoutes.Backup)
  }, [isAndroid, isPanel, history])

  const onShowVisibleAssetsModal = React.useCallback(
    (showModal: boolean) => {
      if (showModal) {
        history.push(WalletRoutes.AddAssetModal)
      } else {
        history.push(WalletRoutes.PortfolioAssets)
      }
    },
    [history]
  )

  const hideVisibleAssetsModal = React.useCallback(
    () => onShowVisibleAssetsModal(false),
    [onShowVisibleAssetsModal]
  )

  // computed
  const isCheckingWallets =
    isCheckingInstalledExtensions ||
    isLoadingDefaultEthereumWallet ||
    isLoadingDefaultSolanaWallet

  const showBanner =
    !isCheckingWallets &&
    (defaultEthereumWallet !== BraveWallet.DefaultWallet.BraveWallet ||
      defaultSolanaWallet !== BraveWallet.DefaultWallet.BraveWallet) &&
    (defaultEthereumWallet !==
      BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
      defaultSolanaWallet !==
        BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
      (defaultEthereumWallet ===
        BraveWallet.DefaultWallet.BraveWalletPreferExtension &&
        isMetaMaskInstalled)) &&
    !isDefaultWalletBannerDismissed

  const noBannerPadding =
    isPanel &&
    (!showBanner ||
      (!isCheckingWalletBackupStatus &&
        !isWalletBackedUp &&
        !isBackupWarningDismissed))

  // memos
  const banners = React.useMemo(
    () => (
      <>
        {showBanner && (
          <WalletBanner
            onDismiss={() => {
              setDismissDefaultWalletBanner(true)
            }}
            onClick={openWalletSettings}
            bannerType='warning'
            buttonText={getLocale('braveWalletWalletPopupSettings')}
            description={getLocale('braveWalletDefaultWalletBanner')}
          />
        )}
        {!isCheckingWalletBackupStatus &&
          !isWalletBackedUp &&
          !isBackupWarningDismissed && (
            <WalletBanner
              onDismiss={() => {
                setDismissBackupWarning(true)
              }}
              onClick={onShowBackup}
              bannerType='danger'
              buttonText={getLocale('braveWalletBackupButton')}
              description={getLocale('braveWalletBackupWarningText')}
            />
          )}
      </>
    ),
    [
      showBanner,
      isCheckingWalletBackupStatus,
      isWalletBackedUp,
      onShowBackup,
      isBackupWarningDismissed
    ]
  )

  // render
  return (
    <>
      <Switch>
        {/* Portfolio */}
        <Route
          path={WalletRoutes.AddAssetModal}
          exact
        >
          {/* Show portfolio overview in background */}
          <WalletPageWrapper
            wrapContentInBox={true}
            noCardPadding={true}
            cardHeader={<PortfolioOverviewHeader />}
            useDarkBackground={isPanel}
            hideDivider={true}
          >
            <StyledWrapper>
              <Column
                fullWidth={true}
                padding={noBannerPadding ? '0px' : '20px 20px 0px 20px'}
              >
                {banners}
              </Column>
              <PortfolioOverview />
            </StyledWrapper>
          </WalletPageWrapper>
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

        <Route path={WalletRoutes.Portfolio}>
          <WalletPageWrapper
            wrapContentInBox={true}
            noCardPadding={true}
            cardHeader={<PortfolioOverviewHeader />}
            useDarkBackground={isPanel}
            hideDivider={isPanel}
          >
            <StyledWrapper>
              <Column
                fullWidth={true}
                padding={noBannerPadding ? '0px' : '20px 20px 0px 20px'}
              >
                {banners}
              </Column>
              <PortfolioOverview />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        {/* Accounts */}
        <Route path={WalletRoutes.AddAccountModal}>
          {/* Show accounts overview in background */}
          <WalletPageWrapper wrapContentInBox={true}>
            <StyledWrapper>
              {banners}
              <Accounts />
            </StyledWrapper>
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
            hideDivider
          >
            <StyledWrapper>
              {banners}
              <MarketView />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.MarketSub}
          exact={true}
        >
          <WalletPageWrapper wrapContentInBox={true}>
            <StyledWrapper>
              {banners}
              <MarketAsset />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        {/* Web3 */}
        <Route
          path={WalletRoutes.Web3}
          exact={true}
        >
          <WalletPageWrapper
            wrapContentInBox
            cardHeader={<ExploreWeb3Header />}
            hideDivider
            noCardPadding
          >
            <StyledWrapper>
              {banners}
              <ExploreWeb3View />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.Explore}
          exact={true}
        >
          <Redirect to={WalletRoutes.Market} />
        </Route>

        {/* Transactions */}
        <Route
          path={WalletRoutes.Activity}
          exact={true}
        >
          <TransactionsScreen />
        </Route>

        <Redirect to={sessionRoute || WalletRoutes.PortfolioAssets} />
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
