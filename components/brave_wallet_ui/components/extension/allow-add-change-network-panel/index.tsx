// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { BraveWallet, SerializableOriginInfo } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'

// Components
import { NavButton, PanelTab } from '..'
import { CreateSiteOrigin } from '../../shared'

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
  originInfo: SerializableOriginInfo
  networkPayload: BraveWallet.NetworkInfo
  panelType: 'add' | 'change'
  onCancel: () => void
  onApproveAddNetwork: () => void
  onApproveChangeNetwork: () => void
}

function AllowAddChangeNetworkPanel (props: Props) {
  const {
    originInfo,
    networkPayload,
    panelType,
    onCancel,
    onApproveAddNetwork,
    onApproveChangeNetwork
  } = props
  const rpcUrl = networkPayload.rpcEndpoints[networkPayload.activeRpcEndpointIndex]?.url || ''
  const blockUrl = networkPayload.blockExplorerUrls.length ? networkPayload.blockExplorerUrls[0] : ''

  const [selectedTab, setSelectedTab] = React.useState<tabs>('network')
  const onSelectTab = (tab: tabs) => () => {
    setSelectedTab(tab)
  }

  const onLearnMore = () => {
    chrome.tabs.create({
      url: 'https://support.brave.com/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ'
    }).catch((e) => { console.error(e) })
  }

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
            : getLocale('braveWalletAllowAddNetworkTitle')
          }
        </PanelTitle>
        <Description>
          {panelType === 'change'
            ? getLocale('braveWalletAllowChangeNetworkDescription')
            : getLocale('braveWalletAllowAddNetworkDescription')}{' '}
          {panelType === 'add' &&
            <DetailsButton
              onClick={onLearnMore}
            >
              {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
            </DetailsButton>
          }
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
            <NetworkTitle>{getLocale('braveWalletAllowAddNetworkName')}</NetworkTitle>
            <NetworkDetail>{networkPayload.chainName}</NetworkDetail>
          </MessageBoxColumn>
          <MessageBoxColumn>
            <NetworkTitle>{getLocale('braveWalletAllowAddNetworkUrl')}</NetworkTitle>
            <NetworkDetail>{rpcUrl}</NetworkDetail>
          </MessageBoxColumn>
          {selectedTab === 'details' &&
            <>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletAllowAddNetworkChainID')}</NetworkTitle>
                <NetworkDetail>{networkPayload.chainId}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletAllowAddNetworkCurrencySymbol')}</NetworkTitle>
                <NetworkDetail>{networkPayload.symbol}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletWatchListTokenDecimals')}</NetworkTitle>
                <NetworkDetail>{networkPayload.decimals}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletAllowAddNetworkExplorer')}</NetworkTitle>
                <NetworkDetail>{blockUrl}</NetworkDetail>
              </MessageBoxColumn>
            </>
          }
        </MessageBox>
      </CenterColumn>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='confirm'
          text={
            panelType === 'change'
              ? getLocale('braveWalletAllowChangeNetworkButton')
              : getLocale('braveWalletAllowAddNetworkButton')
          }
          onSubmit={
            panelType === 'add'
              ? onApproveAddNetwork
              : onApproveChangeNetwork
          }
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default AllowAddChangeNetworkPanel
