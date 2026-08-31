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
import { AccountPageTabs, WalletRoutes } from '../../../constants/types'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'
import { useLockWalletMutation } from '../../../common/slices/api.slice'
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

  // mutations
  const [lockWallet] = useLockWalletMutation()

  // methods
  const onClickConnectedSites = React.useCallback(() => {
    // TODO(https://github.com/brave/brave-browser/issues/58322): Should be
    // able to navigate to specific coin permissions.
    const dappCoinName = 'ethereum'
    const dappPermissionsUrl = `brave://settings/content/${dappCoinName}`

    chrome.tabs.create({ url: dappPermissionsUrl }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

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

  // Memos
  const accountSettingsOptions = React.useMemo(() => {
    if (isMobile) {
      return CreateAccountOptions.filter(
        (option) => option.name !== S.BRAVE_WALLET_CONNECT_HARDWARE_WALLET,
      )
    }
    return CreateAccountOptions
  }, [isMobile])

  const showConnectedSitesItem = !isMobile
  const showOpenWalletSettingsItem = !isMobile

  return (
    <ButtonMenu placement='bottom-end'>
      {children}
      <leo-menu-item
        onClick={async () => {
          await lockWallet()
        }}
      >
        <Icon name='lock' />
        {getLocale(S.BRAVE_WALLET_WALLET_POPUP_LOCK)}
      </leo-menu-item>

      <leo-menu-item onClick={onClickBackup}>
        <Icon name='safe' />
        {getLocale(S.BRAVE_WALLET_WALLET_POPUP_BACKUP)}
      </leo-menu-item>

      {showConnectedSitesItem && (
        <leo-menu-item onClick={onClickConnectedSites}>
          <Icon name='link-normal' />
          {getLocale(S.BRAVE_WALLET_WALLET_POPUP_CONNECTED_SITES)}
        </leo-menu-item>
      )}

      {showOpenWalletSettingsItem && (
        <leo-menu-item onClick={openWalletSettings}>
          <Icon name='settings' />
          {getLocale(S.BRAVE_WALLET_WALLET_POPUP_SETTINGS)}
        </leo-menu-item>
      )}

      {(walletLocation === WalletRoutes.PortfolioNFTs
        || walletLocation === WalletRoutes.PortfolioAssets
        || walletLocation === WalletRoutes.PortfolioActivity) && (
        <>
          <leo-title>{getLocale(S.BRAVE_WALLET_PORTFOLIO_SETTINGS)}</leo-title>
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
              {getLocale(S.BRAVE_WALLET_WALLET_POPUP_HIDE_BALANCES)}
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
              {getLocale(S.BRAVE_WALLET_WALLET_POPUP_SHOW_GRAPH)}
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
              {getLocale(S.BRAVE_WALLET_DISTRIBUTION)}
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
              {getLocale(S.BRAVE_WALLET_WALLET_NFTS_TAB)}
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
          <leo-title>{getLocale(S.BRAVE_WALLET_ACCOUNT_SETTINGS)}</leo-title>
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
        {getLocale(S.BRAVE_WALLET_HELP_CENTER)}
      </leo-menu-item>
    </ButtonMenu>
  )
}
