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
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { LoadingSkeleton } from '../components/shared/loading-skeleton/index'
import { WalletNav } from '../components/desktop/wallet-nav/wallet-nav'
import { NftIpfsBanner } from '../components/desktop/nft-ipfs-banner/nft-ipfs-banner'
import { LocalIpfsNodeScreen } from '../components/desktop/local-ipfs-node/local-ipfs-node'
import { InspectNftsScreen } from '../components/desktop/inspect-nfts/inspect-nfts'
import WalletPageStory from './wrappers/wallet-page-story-wrapper'
import { mockErc721Token, mockNetwork } from '../common/constants/mocks'
import { mockNFTMetadata } from './mock-data/mock-nft-metadata'
import { NftPinningStatus } from '../components/desktop/nft-pinning-status/nft-pinning-status'
import { NftsEmptyState } from '../components/desktop/views/nfts/components/nfts-empty-state/nfts-empty-state'
import { EnableNftDiscoveryModal } from '../components/desktop/popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'
import { NftScreen } from '../nft/components/nft-details/nft-screen'
import { ContainerCard, LayoutCardWrapper } from '../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { NFTGridViewItem } from '../components/desktop/views/portfolio/components/nft-grid-view/nft-grid-view-item'
import { TabOption, Tabs } from '../components/shared/tabs/tabs'
import { AutoDiscoveryEmptyState } from '../components/desktop/views/nfts/components/auto-discovery-empty-state/auto-discovery-empty-state'
import { MarketGrid } from '../components/shared/market-grid/market-grid'
import { marketGridHeaders } from '../options/market-data-headers'
import { coinMarketMockData } from './mock-data/mock-coin-market-data'

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
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')

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

export const _SweepstakesBanner = () => {
  return <SweepstakesBanner
    startDate={new Date(Date.now())}
    endDate={new Date(Date.now() + 1)}
  />
}

_SweepstakesBanner.story = {
  name: 'Sweepstakes Banner'
}

export const _LoadingSkeleton = () => {
  return (
  <div
    style={{
      width: '600px',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    }}>
    <LoadingSkeleton
      width={500}
      height={20}
      count={5}
    />
  </div>)
}

_LoadingSkeleton.story = {
  name: 'Loading Skeleton'
}

export const _BuySendSwapDeposit = () => {
  return (
    <WalletNav />
  )
}

_BuySendSwapDeposit.story = {
  name: 'Buy/Send/Swap/Deposit'
}

export const _NftIpfsBanner = () => {
  const [showBanner, setShowBanner] = React.useState(true)

  const onDismiss = React.useCallback(() => {
    setShowBanner(false)
  }, [])

  return (
    <WalletPageStory>
      <div style={{ width: '855px' }}>
        {showBanner && <NftIpfsBanner onDismiss={onDismiss} />}
      </div>
    </WalletPageStory>
  )
}

_NftIpfsBanner.story = {
  name: 'NFT IPFS Banner'
}

export const _LocalIpfsScreen = () => {
  const onClose = () => {
    console.log('close')
  }

  return (
    <WalletPageStory>
      <LocalIpfsNodeScreen
        onClose={onClose}
      />
    </WalletPageStory>
  )
}

_LocalIpfsScreen.story = {
  name: 'Run Local IPFS Node'
}

export const _InspectNftsScreen = () => {
  const onClose = () => {
    console.log('on close')
  }
  const onBack = () => {
    console.log('on back')
  }
  return (
    <WalletPageStory>
      <InspectNftsScreen
        onClose={onClose}
        onBack={onBack}
      />
    </WalletPageStory>
  )
}

_InspectNftsScreen.story = {
  name: 'Inspect NFTs Screen'
}

export const _NftPinningStatus = () => {
  return (
    <div style={{ display: 'grid', gap: 10 }}>
      {/* uploading */}
      <NftPinningStatus
        pinningStatusCode={3}
      />

      {/* success */}
      <NftPinningStatus
        pinningStatusCode={2}
      />

      {/* failed */}
      <NftPinningStatus
        pinningStatusCode={4}
      />
    </div>
  )
}

_NftPinningStatus.story = {
  title: 'NFT Pinning Status'
}

export const _NftsEmptyState = () => {
  return (
    <NftsEmptyState
      onImportNft={() => console.log('On import NFT')}
    />
  )
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
        nftMetadataError: '',
        selectedAsset: mockErc721Token,
      }}
    >
      <LayoutCardWrapper
        headerHeight={92}
      >
        <ContainerCard
        >
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

export const _NFTGridViewItem = () => {
  return (
    <WalletPageStory>
      <NFTGridViewItem
        isTokenHidden={false}
        isTokenSpam={false}
        token={mockErc721Token}
        onSelectAsset={() => {}}
        networks={[]}
        accounts={[]}
      />
    </WalletPageStory>
  )
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
        onSort={(columnId, sortOrder) => console.log(`sort by ${columnId} ${sortOrder}`)}
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
