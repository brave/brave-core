// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router-dom'
import Toggle from '@brave/leo/react/toggle'

// Types
import { WalletRoutes } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { useSyncedLocalStorage } from '../../../common/hooks/use_local_storage'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../common/constants/local-storage-keys'

// Styled Components
import {
  StyledWrapper,
  PopupButtonText,
  ButtonIcon,
  ToggleRow
} from './wellet-menus.style'
import { Row } from '../../shared/style'

export const PortfolioOverviewMenu = () => {
  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // Local-Storage
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

  return (
    <StyledWrapper yPosition={42}>
      <ToggleRow onClick={onToggleHideBalances}>
        <Row>
          <ButtonIcon name='eye-on' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupHideBalances')}
          </PopupButtonText>
          <Toggle
            checked={!hidePortfolioBalances}
            onChange={onToggleHideBalances}
            size='small'
          />
        </Row>
      </ToggleRow>

      <ToggleRow onClick={onToggleHideGraph}>
        <Row>
          <ButtonIcon name='graph' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupShowGraph')}
          </PopupButtonText>
        </Row>
        <Toggle
          checked={!hidePortfolioGraph}
          onChange={onToggleHideGraph}
          size='small'
        />
      </ToggleRow>

      <ToggleRow onClick={onToggleHideNFTsTab}>
        <Row>
          <ButtonIcon name='nft' />
          <PopupButtonText>
            {getLocale('braveWalletWalletNFTsTab')}
          </PopupButtonText>
        </Row>
        <Toggle
          checked={!hidePortfolioNFTsTab}
          onChange={onToggleHideNFTsTab}
          size='small'
        />
      </ToggleRow>
    </StyledWrapper>
  )
}

export default PortfolioOverviewMenu
