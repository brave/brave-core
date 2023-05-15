// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory, useLocation } from 'react-router-dom'
import Checkbox from '@brave/leo/react/checkbox'

// Types
import { WalletRoutes } from '../../../constants/types'

// Actions
import { WalletActions } from '../../../common/actions'

// Selectors
import {
  useSafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import {
  WalletSelectors
} from '../../../common/selectors'

// Utils
import { getLocale } from '../../../../common/locale'

// Constants
import {
  LOCAL_STORAGE_KEYS
} from '../../../common/constants/local-storage-keys'

// Styled Components
import {
  StyledWrapper,
  PopupButtonText,
  ButtonIcon,
  CheckBoxRow
} from './wellet-menus.style'
import {
  Row
} from '../../shared/style'

export const PortfolioOverviewMenu = () => {
  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // Redux
  const dispatch = useDispatch()

  const hidePortfolioGraph =
    useSafeWalletSelector(WalletSelectors.hidePortfolioGraph)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)
  const hidePortfolioNFTsTab =
    useSafeWalletSelector(WalletSelectors.hidePortfolioNFTsTab)

  // Methods
  const onToggleHideGraph = React.useCallback(() => {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
      hidePortfolioGraph
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioGraph(
          !hidePortfolioGraph
        ))
  }, [hidePortfolioGraph])

  const onToggleHideBalances = React.useCallback(() => {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
      hidePortfolioBalances
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioBalances(
          !hidePortfolioBalances
        ))
  }, [hidePortfolioBalances])

  const onToggleHideNFTsTab = React.useCallback(() => {
    if (walletLocation.includes(WalletRoutes.PortfolioNFTs)) {
      history.push(WalletRoutes.PortfolioAssets)
    }
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
      hidePortfolioNFTsTab
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioNFTsTab(
          !hidePortfolioNFTsTab
        ))
  }, [hidePortfolioNFTsTab, walletLocation])

  return (
    <StyledWrapper yPosition={42}>
      <CheckBoxRow onClick={onToggleHideBalances}>
        <ButtonIcon name='eye-off' />
        <PopupButtonText>
          {getLocale('braveWalletWalletPopupHideBalances')}
        </PopupButtonText>
        <Checkbox
          checked={hidePortfolioBalances}
          onChanged={onToggleHideBalances}
          size='normal'
        />
      </CheckBoxRow>

      <CheckBoxRow onClick={onToggleHideGraph}>
        <Row>
          {/* This graph icon needs to be updated to the
              one in figma once it is added to leo. */}
          <ButtonIcon name='graph' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupShowGraph')}
          </PopupButtonText>
        </Row>
        <Checkbox
          checked={!hidePortfolioGraph}
          onChanged={onToggleHideGraph}
          size='normal'
        />
      </CheckBoxRow>

      <CheckBoxRow onClick={onToggleHideNFTsTab}>
        <Row>
          <ButtonIcon name='nft' />
          <PopupButtonText>
            {getLocale('braveWalletWalletNFTsTab')}
          </PopupButtonText>
        </Row>
        <Checkbox
          checked={!hidePortfolioNFTsTab}
          onChanged={onToggleHideNFTsTab}
          size='normal'
        />
      </CheckBoxRow>

    </StyledWrapper>
  )
}

export default PortfolioOverviewMenu
