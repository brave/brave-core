// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch } from 'react-redux'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { PanelActions } from '../../../panel/actions'
import {
  useUnsafePanelSelector //
} from '../../../common/hooks/use-safe-selector'
import { PanelSelectors } from '../../../panel/selectors'

// Components
import { NavButton } from '../buttons/nav-button'
import { PanelTab } from '../panel-tab/index'
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'

// Styled Components
import {
  MessageBox,
  NetworkTitle,
  MessageBoxColumn,
  DetailsButton,
  ButtonRow,
  FavIcon,
  NetworkDetail,
  TabRow
} from './style'

import {
  StyledWrapper,
  CenterColumn,
  Description,
  PanelTitle,
  URLText
} from '../shared-panel-styles'

export type tabs = 'network' | 'details'

export interface Props {
  originInfo: BraveWallet.OriginInfo
  networkPayload: BraveWallet.NetworkInfo
  panelType: 'add' | 'change'
}

const onLearnMore = () => {
  chrome.tabs
    .create({
      url:
        'https://support.brave.com' +
        '/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ'
    })
    .catch((e) => {
      console.error(e)
    })
}

export function AllowAddChangeNetworkPanel(props: Props) {
  const { originInfo, networkPayload, panelType } = props

  // computed from props
  const rpcUrl =
    networkPayload.rpcEndpoints[networkPayload.activeRpcEndpointIndex]?.url ||
    ''
  const blockUrl = networkPayload.blockExplorerUrls.length
    ? networkPayload.blockExplorerUrls[0]
    : ''

  // redux
  const dispatch = useDispatch()
  const addChainRequest = useUnsafePanelSelector(PanelSelectors.addChainRequest)
  const switchChainRequest = useUnsafePanelSelector(
    PanelSelectors.switchChainRequest
  )

  // state
  const [selectedTab, setSelectedTab] = React.useState<tabs>('network')

  // methods
  const onSelectTab = (tab: tabs) => () => {
    setSelectedTab(tab)
  }

  const onApproveAddNetwork = () => {
    dispatch(
      PanelActions.addEthereumChainRequestCompleted({
        chainId: addChainRequest.networkInfo.chainId,
        approved: true
      })
    )
  }

  const onApproveChangeNetwork = () => {
    dispatch(
      PanelActions.switchEthereumChainProcessed({
        requestId: switchChainRequest.requestId,
        approved: true
      })
    )
  }

  const onCancelAddNetwork = () => {
    dispatch(
      PanelActions.addEthereumChainRequestCompleted({
        chainId: addChainRequest.networkInfo.chainId,
        approved: false
      })
    )
  }

  const onCancelChangeNetwork = () => {
    dispatch(
      PanelActions.switchEthereumChainProcessed({
        requestId: switchChainRequest.requestId,
        approved: false
      })
    )
  }

  // render
  return (
    <StyledWrapper>
      <CenterColumn>
        <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`} />
        <URLText>
          <CreateSiteOrigin
            originSpec={originInfo.originSpec}
            eTldPlusOne={originInfo.eTldPlusOne}
          />
        </URLText>
        <PanelTitle>
          {panelType === 'change'
            ? getLocale('braveWalletAllowChangeNetworkTitle')
            : getLocale('braveWalletAllowAddNetworkTitle')}
        </PanelTitle>
        <Description>
          {panelType === 'change'
            ? getLocale('braveWalletAllowChangeNetworkDescription')
            : getLocale('braveWalletAllowAddNetworkDescription')}{' '}
          {panelType === 'add' && (
            <DetailsButton onClick={onLearnMore}>
              {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
            </DetailsButton>
          )}
        </Description>
        <TabRow>
          <PanelTab
            isSelected={selectedTab === 'network'}
            onSubmit={onSelectTab('network')}
            text={getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')}
          />
          <PanelTab
            isSelected={selectedTab === 'details'}
            onSubmit={onSelectTab('details')}
            text={getLocale('braveWalletAllowAddNetworkDetailsPanelTitle')}
          />
        </TabRow>
        <MessageBox>
          <MessageBoxColumn>
            <NetworkTitle>
              {getLocale('braveWalletAllowAddNetworkName')}
            </NetworkTitle>
            <NetworkDetail>{networkPayload.chainName}</NetworkDetail>
          </MessageBoxColumn>
          <MessageBoxColumn>
            <NetworkTitle>
              {getLocale('braveWalletAllowAddNetworkUrl')}
            </NetworkTitle>
            <NetworkDetail>{rpcUrl}</NetworkDetail>
          </MessageBoxColumn>
          {selectedTab === 'details' && (
            <>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletChainId')}</NetworkTitle>
                <NetworkDetail>{networkPayload.chainId}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>
                  {getLocale('braveWalletAllowAddNetworkCurrencySymbol')}
                </NetworkTitle>
                <NetworkDetail>{networkPayload.symbol}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>
                  {getLocale('braveWalletWatchListTokenDecimals')}
                </NetworkTitle>
                <NetworkDetail>{networkPayload.decimals}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>
                  {getLocale('braveWalletAllowAddNetworkExplorer')}
                </NetworkTitle>
                <NetworkDetail>{blockUrl}</NetworkDetail>
              </MessageBoxColumn>
            </>
          )}
        </MessageBox>
      </CenterColumn>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={
            panelType === 'add' ? onCancelAddNetwork : onCancelChangeNetwork
          }
        />
        <NavButton
          buttonType='confirm'
          text={
            panelType === 'change'
              ? getLocale('braveWalletAllowChangeNetworkButton')
              : getLocale('braveWalletAllowAddNetworkButton')
          }
          onSubmit={
            panelType === 'add' ? onApproveAddNetwork : onApproveChangeNetwork
          }
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default AllowAddChangeNetworkPanel
