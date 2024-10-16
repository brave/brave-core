// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { DesktopComponentWrapper, DesktopComponentWrapperRow } from './style'
import { SideNav } from '../components/desktop/side-nav/index'
import { TopTabNav } from '../components/desktop/top-tab-nav/index'
import { NavTypes, TopTabNavTypes } from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import { TopNavOptions } from '../options/top-nav-options'
import './locale'
import { LoadingSkeleton } from '../components/shared/loading-skeleton/index'
import { WalletNav } from '../components/desktop/wallet-nav/wallet-nav'
import WalletPageStory from './wrappers/wallet-page-story-wrapper'
import { mockNetwork } from '../common/constants/mocks'
import { mockNFTMetadata } from './mock-data/mock-nft-metadata'
import { NftsEmptyState } from '../components/desktop/views/nfts/components/nfts-empty-state/nfts-empty-state'
import { EnableNftDiscoveryModal } from '../components/desktop/popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { NftScreen } from '../nft/components/nft-details/nft-screen'
import {
  ContainerCard,
  LayoutCardWrapper
} from '../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { TabOption, Tabs } from '../components/shared/tabs/tabs'
import { AutoDiscoveryEmptyState } from '../components/desktop/views/nfts/components/auto-discovery-empty-state/auto-discovery-empty-state'
import { MarketGrid } from '../components/shared/market-grid/market-grid'
import { marketGridHeaders } from '../options/market-data-headers'
import { coinMarketMockData } from './mock-data/mock-coin-market-data'
import { mockErc721Token } from './mock-data/mock-asset-options'

export default {
  title: 'Wallet/Desktop/Components',
  parameters: {
    layout: 'centered'
  }
}

export const _DesktopSideNav = () => {
  const [selectedButton, setSelectedButton] = React.useState<NavTypes>('crypto')

  const navigateTo = (path: NavTypes) => {
    setSelectedButton(path)
  }

  return (
    <DesktopComponentWrapper>
      <SideNav
        navList={NavOptions()}
        selectedButton={selectedButton}
        onSubmit={navigateTo}
      />
    </DesktopComponentWrapper>
  )
}

_DesktopSideNav.story = {
  name: 'Side Nav'
}

export const _DesktopTopTabNav = () => {
  const [selectedTab, setSelectedTab] =
    React.useState<TopTabNavTypes>('portfolio')

  const onSelectTab = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  return (
    <DesktopComponentWrapperRow>
      <TopTabNav
        tabList={TopNavOptions()}
        selectedTab={selectedTab}
        onSelectTab={onSelectTab}
      />
    </DesktopComponentWrapperRow>
  )
}

_DesktopTopTabNav.story = {
  name: 'Top Tab Nav'
}

export const _LoadingSkeleton = () => {
  return (
    <div
      style={{
        width: '600px',
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center'
      }}
    >
      <LoadingSkeleton
        width={500}
        height={20}
        count={5}
      />
    </div>
  )
}

_LoadingSkeleton.story = {
  name: 'Loading Skeleton'
}

export const _BuySendSwapDeposit = () => {
  return <WalletNav isAndroid={false} />
}

_BuySendSwapDeposit.story = {
  name: 'Buy/Send/Swap/Deposit'
}

export const _NftsEmptyState = () => {
  return <NftsEmptyState onImportNft={() => console.log('On import NFT')} />
}

export const _EnableNftDiscoveryModal = () => {
  return (
    <EnableNftDiscoveryModal
      onCancel={() => {}}
      onConfirm={() => {}}
    />
  )
}

_EnableNftDiscoveryModal.story = {
  title: 'Enable NFT Discovery Modal'
}

export const _NftScreen = () => {
  return (
    <WalletPageStory
      pageStateOverride={{
        isAutoPinEnabled: true,
        isFetchingNFTMetadata: false,
        nftMetadata: mockNFTMetadata[0],
        nftMetadataError: ''
      }}
    >
      <LayoutCardWrapper headerHeight={92}>
        <ContainerCard>
          <NftScreen
            selectedAsset={mockErc721Token}
            tokenNetwork={mockNetwork}
          />
        </ContainerCard>
      </LayoutCardWrapper>
    </WalletPageStory>
  )
}

export const _AutoDiscoveryEmptyState = () => {
  return (
    <AutoDiscoveryEmptyState
      onImportNft={() => console.log('Import NFT')}
      onRefresh={() => console.log('Import NFT')}
      isRefreshingTokens={false}
    />
  )
}

_AutoDiscoveryEmptyState.story = {
  title: 'NFT Auto Discovery Empty State'
}

export const _Tabs = () => {
  const options: TabOption[] = [
    {
      id: 'nfts',
      label: 'NFTs',
      labelSummary: '10'
    },
    {
      id: 'hidden',
      label: 'Hidden'
    }
  ]
  return (
    <Tabs
      options={options}
      onSelect={(option) => console.log(option)}
    />
  )
}

export const _MarketGrid = () => {
  return (
    <div
      style={{
        width: '700px'
      }}
    >
      <MarketGrid
        headers={marketGridHeaders}
        coinMarketData={coinMarketMockData}
        showEmptyState={false}
        sortedBy='marketCap'
        onSort={(columnId, sortOrder) =>
          console.log(`sort by ${columnId} ${sortOrder}`)
        }
        onSelectCoinMarket={() => {}}
        isBuySupported={() => true}
        isDepositSupported={() => false}
        onClickBuy={() => {}}
        onClickDeposit={() => {}}
        onUpdateIframeHeight={() => {}}
        fiatCurrency={'USD'}
      />
    </div>
  )
}
