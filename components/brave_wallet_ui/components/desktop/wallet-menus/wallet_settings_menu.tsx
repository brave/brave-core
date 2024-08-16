// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import Toggle from '@brave/leo/react/toggle'
import * as leo from '@brave/leo/tokens/css/variables'

// Selectors
import { UISelectors } from '../../../common/selectors'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Types
import {
  AccountPageTabs,
  BraveWallet,
  WalletRoutes
} from '../../../constants/types'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../common/constants/local-storage-keys'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  useGetSelectedChainQuery,
  useLockWalletMutation
} from '../../../common/slices/api.slice'
import { openWalletSettings } from '../../../utils/routes-utils'
import { useSyncedLocalStorage } from '../../../common/hooks/use_local_storage'

// Styled Components
import { VerticalDivider } from '../../shared/style'
import {
  MenuItemIcon,
  ButtonMenu,
  MenuButton,
  MenuItemRow,
  MenuOptionRow,
  MoreVerticalIcon,
  PopupButton,
  SectionLabel,
  ToggleRow
} from './wallet_menus.style'

export interface Props {
  anchorButtonKind?: 'filled' | 'plain-faint'
}

const onClickHelpCenter = () => {
  chrome.tabs.create(
    {
      url: 'https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet'
    },
    () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    }
  )
}

export const WalletSettingsMenu = ({
  anchorButtonKind = 'plain-faint'
}: Props) => {
  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // local-storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)
  const [hidePortfolioNFTsTab, setHidePortfolioNFTsTab] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
    false
  )
  const [hidePortfolioGraph, setHidePortfolioGraph] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
    false
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

    const route =
      selectedNetwork.coin === BraveWallet.CoinType.ETH ? 'ethereum' : 'solana'

    chrome.tabs.create({ url: `brave://settings/content/${route}` }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [selectedNetwork])

  // Methods
  const onToggleHideGraph = React.useCallback(() => {
    setHidePortfolioGraph((prev) => !prev)
  }, [setHidePortfolioGraph])

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
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      })
      return
    }
    history.push(route)
  }

  const onClickBackup = () => {
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
  }

  return (
    <ButtonMenu>
      <div slot='anchor-content'>
        <MenuButton
          kind={anchorButtonKind}
          padding={anchorButtonKind === 'filled' ? '1px 2px' : '0px'}
          buttonColor={leo.color.container.background}
        >
          <MoreVerticalIcon />
        </MenuButton>
      </div>
      <PopupButton
        onClick={async () => {
          await lockWallet()
        }}
      >
        <MenuItemRow>
          <MenuItemIcon name='lock' />
          {getLocale('braveWalletWalletPopupLock')}
        </MenuItemRow>
      </PopupButton>

      <PopupButton onClick={onClickBackup}>
        <MenuItemRow>
          <MenuItemIcon name='safe' />
          {getLocale('braveWalletBackupButton')}
        </MenuItemRow>
      </PopupButton>

      {(selectedNetwork?.coin === BraveWallet.CoinType.ETH ||
        selectedNetwork?.coin === BraveWallet.CoinType.SOL) && (
        <PopupButton onClick={onClickConnectedSites}>
          <MenuItemRow>
            <MenuItemIcon name='link-normal' />
            {getLocale('braveWalletWalletPopupConnectedSites')}
          </MenuItemRow>
        </PopupButton>
      )}

      <PopupButton onClick={openWalletSettings}>
        <MenuItemRow>
          <MenuItemIcon name='settings' />
          {getLocale('braveWalletWalletPopupSettings')}
        </MenuItemRow>
      </PopupButton>

      {/* {(walletLocation === WalletRoutes.PortfolioNFTs ||
        walletLocation === WalletRoutes.PortfolioAssets) && ( */}
      <>
        <SectionLabel justifyContent='flex-start'>
          {getLocale('braveWalletPortfolioSettings')}
        </SectionLabel>

        <ToggleRow>
          <MenuOptionRow onClick={onToggleHideBalances}>
            <MenuItemRow>
              <MenuItemIcon name='eye-on' />
              {getLocale('braveWalletWalletPopupHideBalances')}
            </MenuItemRow>
            <Toggle
              checked={!hidePortfolioBalances}
              onChange={onToggleHideBalances}
              size='small'
            />
          </MenuOptionRow>
        </ToggleRow>

        <ToggleRow>
          <MenuOptionRow onClick={onToggleHideGraph}>
            <MenuItemRow>
              <MenuItemIcon name='graph' />
              {getLocale('braveWalletWalletPopupShowGraph')}
            </MenuItemRow>
            <Toggle
              checked={!hidePortfolioGraph}
              onChange={onToggleHideGraph}
              size='small'
            />
          </MenuOptionRow>
        </ToggleRow>

        <ToggleRow>
          <MenuOptionRow onClick={onToggleHideNFTsTab}>
            <MenuItemRow>
              <MenuItemIcon name='nft' />
              {getLocale('braveWalletWalletNFTsTab')}
            </MenuItemRow>
            <Toggle
              checked={!hidePortfolioNFTsTab}
              onChange={onToggleHideNFTsTab}
              size='small'
            />
          </MenuOptionRow>
        </ToggleRow>
      </>
      {/* )} */}

      {walletLocation === WalletRoutes.Accounts && isPanel && (
        <>
          <SectionLabel justifyContent='flex-start'>
            {getLocale('braveWalletAccountSettings')}
          </SectionLabel>

          {CreateAccountOptions.map((option) => (
            <PopupButton
              key={option.name}
              onClick={() => onClickRoute(option.route)}
              minWidth={240}
            >
              <MenuItemRow>
                <MenuItemIcon name={option.icon} />
                {getLocale(option.name)}
              </MenuItemRow>
            </PopupButton>
          ))}
        </>
      )}

      <VerticalDivider />

      <PopupButton onClick={onClickHelpCenter}>
        <MenuItemRow>
          <MenuItemIcon name='help-outline' />
          {getLocale('braveWalletHelpCenter')}
        </MenuItemRow>
      </PopupButton>
    </ButtonMenu>
  )
}
