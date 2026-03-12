// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './leo-menu-elements'
import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import Toggle from '@brave/leo/react/toggle'
import Icon from '@brave/leo/react/icon'

// Page API Proxy
import getWalletPageApiProxy from '../../../page/wallet_page_api_proxy'

// Selectors
import { UISelectors } from '../../../common/selectors'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Types
import {
  AccountPageTabs,
  BraveWallet,
  WalletRoutes,
} from '../../../constants/types'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  useGetSelectedChainQuery,
  useLockWalletMutation,
} from '../../../common/slices/api.slice'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import { PanelActions } from '../../../panel/actions'
import { openWalletSettings } from '../../../utils/routes-utils'
import { useSyncedLocalStorage } from '../../../common/hooks/use_local_storage'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'
import { Row } from '../../shared/style'

export interface Props {
  children: React.ReactNode
}

const HELP_CENTER_URL =
  'https://support.brave.app/hc/categories/360001062531-Wallet'

export const WalletSettingsMenu = (props: Props) => {
  const { children } = props

  // Redux
  const dispatch = useAppDispatch()

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)
  const isMobileOrPanel = isMobile || isPanel

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // local-storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)
  const [hidePortfolioNFTsTab, setHidePortfolioNFTsTab] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
    false,
  )
  const [hidePortfolioGraph, setHidePortfolioGraph] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
    true,
  )
  const [hidePortfolioDistribution, setHidePortfolioDistribution] =
    useSyncedLocalStorage(
      LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_DISTRIBUTION_HIDDEN,
      true,
    )
  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // mutations
  const [lockWallet] = useLockWalletMutation()

  // methods
  const onClickConnectedSites = React.useCallback(() => {
    if (!selectedNetwork) {
      return
    }

    let route: string
    switch (selectedNetwork.coin) {
      case BraveWallet.CoinType.ETH:
        route = 'ethereum'
        break
      case BraveWallet.CoinType.SOL:
        route = 'solana'
        break
      case BraveWallet.CoinType.ADA:
        route = 'cardano'
        break
      default:
        throw new Error('Coin not supported')
    }

    chrome.tabs.create({ url: `brave://settings/content/${route}` }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [selectedNetwork])

  const onClickHelpCenter = () => {
    if (chrome.tabs !== undefined) {
      chrome.tabs.create(
        {
          url: HELP_CENTER_URL,
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message,
            )
          }
        },
      )
    } else {
      // Tabs.create is desktop specific. Using window.open for mobile
      window.open(HELP_CENTER_URL, '_blank', 'noopener noreferrer')
    }
  }

  // Methods
  const onToggleHideGraph = React.useCallback(() => {
    setHidePortfolioGraph((prev) => !prev)
  }, [setHidePortfolioGraph])

  const onToggleHideDistribution = React.useCallback(() => {
    setHidePortfolioDistribution((prev) => !prev)
  }, [setHidePortfolioDistribution])

  const onToggleHideBalances = React.useCallback(() => {
    setHidePortfolioBalances((prev) => !prev)
  }, [setHidePortfolioBalances])

  const onToggleHideNFTsTab = React.useCallback(() => {
    if (walletLocation.includes(WalletRoutes.PortfolioNFTs)) {
      history.push(WalletRoutes.PortfolioAssets)
    }
    setHidePortfolioNFTsTab((prev) => !prev)
  }, [history, setHidePortfolioNFTsTab, walletLocation])

  const onClickRoute = (route: WalletRoutes | AccountPageTabs) => {
    if (route === WalletRoutes.AddHardwareAccountModalStart && isPanel) {
      chrome.tabs.create({ url: `chrome://wallet${route}` }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message,
          )
        }
      })
      return
    }
    history.push(route)
  }

  const onClickBackup = React.useCallback(() => {
    if (isMobile) {
      getWalletPageApiProxy().pageHandler.showWalletBackupUI()
      return
    }

    if (isPanel) {
      chrome.tabs.create(
        {
          url: `chrome://wallet${WalletRoutes.Backup}`,
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message,
            )
          }
        },
      )
      return
    }
    history.push(WalletRoutes.Backup)
  }, [isMobile, isPanel, history])

  const onClickSnaps = React.useCallback(() => {
    if (isPanel) {
      dispatch(PanelActions.navigateTo('snaps'))
    } else {
      history.push(WalletRoutes.SnapsStore)
    }
  }, [isPanel, dispatch, history])

  // Memos
  const accountSettingsOptions = React.useMemo(() => {
    if (isMobile) {
      return CreateAccountOptions.filter(
        (option) => option.name !== 'braveWalletConnectHardwareWallet',
      )
    }
    return CreateAccountOptions
  }, [isMobile])

  return (
    <ButtonMenu placement='bottom-end'>
      {children}
      <leo-menu-item
        onClick={async () => {
          await lockWallet()
        }}
      >
        <Icon name='lock' />
        {getLocale('braveWalletWalletPopupLock')}
      </leo-menu-item>

      <leo-menu-item onClick={onClickBackup}>
        <Icon name='safe' />
        {getLocale('braveWalletWalletPopupBackup')}
      </leo-menu-item>

      <leo-menu-item onClick={onClickSnaps}>
        <Icon name='web3' />
        Snaps
      </leo-menu-item>

      {(selectedNetwork?.coin === BraveWallet.CoinType.ETH
        || selectedNetwork?.coin === BraveWallet.CoinType.SOL)
        && !isMobile && (
          <leo-menu-item onClick={onClickConnectedSites}>
            <Icon name='link-normal' />
            {getLocale('braveWalletWalletPopupConnectedSites')}
          </leo-menu-item>
        )}

      {!isMobile && (
        <leo-menu-item onClick={openWalletSettings}>
          <Icon name='settings' />
          {getLocale('braveWalletWalletPopupSettings')}
        </leo-menu-item>
      )}

      {(walletLocation === WalletRoutes.PortfolioNFTs
        || walletLocation === WalletRoutes.PortfolioAssets
        || walletLocation === WalletRoutes.PortfolioActivity) && (
        <>
          <leo-title>{getLocale('braveWalletPortfolioSettings')}</leo-title>
          <leo-menu-item
            id='toggle'
            onClick={onToggleHideBalances}
            data-is-interactive='true'
          >
            <Row
              gap='16px'
              width='unset'
            >
              <Icon name='eye-on' />
              {getLocale('braveWalletWalletPopupHideBalances')}
            </Row>
            <Toggle
              checked={!hidePortfolioBalances}
              onChange={onToggleHideBalances}
              size='small'
            />
          </leo-menu-item>

          <leo-menu-item
            id='toggle'
            onClick={onToggleHideGraph}
            data-is-interactive='true'
          >
            <Row
              gap='16px'
              width='unset'
            >
              <Icon name='graph' />
              {getLocale('braveWalletWalletPopupShowGraph')}
            </Row>
            <Toggle
              checked={!hidePortfolioGraph}
              onChange={onToggleHideGraph}
              size='small'
            />
          </leo-menu-item>

          <leo-menu-item
            id='toggle'
            onClick={onToggleHideDistribution}
            data-is-interactive='true'
          >
            <Row
              gap='16px'
              width='unset'
            >
              <Icon name='pie-chart-2' />
              {getLocale('braveWalletDistribution')}
            </Row>
            <Toggle
              checked={!hidePortfolioDistribution}
              onChange={onToggleHideDistribution}
              size='small'
            />
          </leo-menu-item>

          <leo-menu-item
            id='toggle'
            onClick={onToggleHideNFTsTab}
            data-is-interactive='true'
          >
            <Row
              gap='16px'
              width='unset'
            >
              <Icon name='nft' />
              {getLocale('braveWalletWalletNFTsTab')}
            </Row>
            <Toggle
              checked={!hidePortfolioNFTsTab}
              onChange={onToggleHideNFTsTab}
              size='small'
            />
          </leo-menu-item>
        </>
      )}

      {walletLocation === WalletRoutes.Accounts && isMobileOrPanel && (
        <>
          <leo-title>{getLocale('braveWalletAccountSettings')}</leo-title>
          {accountSettingsOptions.map((option) => (
            <leo-menu-item
              key={option.name}
              onClick={() => onClickRoute(option.route)}
            >
              <Icon name={option.icon} />
              {getLocale(option.name)}
            </leo-menu-item>
          ))}
        </>
      )}
      <hr />
      <leo-menu-item onClick={onClickHelpCenter}>
        <Icon name='help-outline' />
        {getLocale('braveWalletHelpCenter')}
      </leo-menu-item>
    </ButtonMenu>
  )
}
