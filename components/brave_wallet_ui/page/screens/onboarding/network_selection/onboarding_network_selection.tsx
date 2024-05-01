// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { EntityId } from '@reduxjs/toolkit'
import { useHistory } from 'react-router'
import Checkbox from '@brave/leo/react/checkbox'
import ProgressRing from '@brave/leo/react/progressRing'
import * as leo from '@brave/leo/tokens/css'

// context
import {
  ChainSelectionContextProvider,
  useChainSelectionContext
} from '../../../../common/context/network_selection_context'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  getNetworkId //
} from '../../../../common/slices/entities/network.entity'
import {
  getOnboardingTypeFromPath,
  openNetworkSettings
} from '../../../../utils/routes-utils'
import { useLocationPathName } from '../../../../common/hooks/use-pathname'

// queries
import {
  useGetAllKnownNetworksQuery,
  useHideNetworksMutation,
  useRestoreNetworksMutation
} from '../../../../common/slices/api.slice'

// types
import {
  BraveWallet,
  SupportedTestNetworkEntityIds,
  SupportedTestNetworks,
  WalletRoutes
} from '../../../../constants/types'

// components
import { SearchBar } from '../../../../components/shared/search-bar'
import {
  CreateNetworkIcon //
} from '../../../../components/shared/create-network-icon'
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'

// styles
import {
  Column,
  MutedLinkText,
  Row,
  Text,
  VerticalSpace
} from '../../../../components/shared/style'
import { ContinueButton, NextButtonRow } from '../onboarding.style'
import {
  SelectAllText,
  GroupingText,
  NetworkSelectionContainer,
  NetworkSelectionGrid,
  ScrollableColumn
} from './onboarding_network_selection.style'
import { WalletActions } from '../../../../common/actions'

// Featured Networks
const featuredChainIds = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID,
  BraveWallet.FILECOIN_MAINNET,
  BraveWallet.BITCOIN_MAINNET,
  BraveWallet.Z_CASH_MAINNET
]

/** Forcing ETH, SOL account creation until their networks can be hidden */
const mandatoryChainIds = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID
]

function NetworkCheckbox({
  network,
  isDisabled
}: {
  network: BraveWallet.NetworkInfo
  isDisabled?: boolean
}) {
  // context
  const [selectedChains, selectChains] = useChainSelectionContext()

  // computed
  const chainIdentifier = getNetworkId(network)
  const isChecked = selectedChains.includes(chainIdentifier)

  // methods
  const onChange = React.useCallback(
    () =>
      selectChains((prev) =>
        isChecked
          ? prev.filter((id) => id !== chainIdentifier)
          : prev.concat(chainIdentifier)
      ),
    [isChecked, selectChains, chainIdentifier]
  )

  // render
  return (
    <NetworkSelectionContainer
      disabled={isDisabled}
      onClick={onChange}
    >
      <Checkbox
        isDisabled={isDisabled}
        checked={isChecked}
        onChange={onChange}
      />

      <CreateNetworkIcon
        network={network}
        marginRight={0}
        size='big'
      />
      <Text
        textSize='14px'
        isBold={false}
      >
        {network.chainName}
      </Text>
    </NetworkSelectionContainer>
  )
}

export const OnboardingNetworkSelection = () => {
  // routing
  const history = useHistory()
  const path = useLocationPathName()
  const onboardingType = getOnboardingTypeFromPath(path)

  // redux
  const dispatch = useDispatch()

  // state
  const [searchText, setSearchText] = React.useState('')
  const [showTestNets, setShowTestNets] = React.useState(false)
  const [selectedChainIds, setSelectedChainIds] = React.useState<EntityId[]>([])

  // queries
  const { data: networks = [], isLoading: isLoadingNetworks } =
    useGetAllKnownNetworksQuery()

  // mutations
  const [hideNetworks] = useHideNetworksMutation()
  const [restoreNetworks] = useRestoreNetworksMutation()

  // memos
  const networkIds = React.useMemo(() => {
    return networks.map(getNetworkId)
  }, [networks])

  /** Always include test networks */
  const selectedChainsContext = React.useMemo(
    () => [selectedChainIds, setSelectedChainIds] as const,
    [selectedChainIds, setSelectedChainIds]
  )

  const mainnetChainIds = React.useMemo(
    () =>
      networkIds.filter((id) => !SupportedTestNetworkEntityIds.includes(id)),
    [networkIds]
  )

  // filter out test networks if needed
  const visibleSelectedChainIds = React.useMemo(() => {
    return showTestNets
      ? selectedChainIds
      : selectedChainIds.filter(
          (id) => !SupportedTestNetworkEntityIds.includes(id)
        )
  }, [showTestNets, selectedChainIds])

  const { featuredNetworks, popularNetworks } = React.useMemo(() => {
    if (!networks) {
      return {
        featuredNetworks: [],
        popularNetworks: []
      }
    }
    // group networks
    const featuredNetworks = featuredChainIds
      .map((id) => networks.find((net) => net.chainId === id))
      .filter((net): net is BraveWallet.NetworkInfo => Boolean(net))

    const popularNetworks = networks.filter(
      (net) => !featuredChainIds.includes(net.chainId)
    )

    // sort popular networks
    popularNetworks.sort((a, b) => a.chainName.localeCompare(b.chainName))

    // remove test networks if needed
    return {
      featuredNetworks,
      popularNetworks: showTestNets
        ? popularNetworks
        : popularNetworks.filter(
            ({ chainId }) => !SupportedTestNetworks.includes(chainId)
          )
    }
  }, [networks, showTestNets])

  const { filteredFeaturedNetworks, filteredPopularNetworks } =
    React.useMemo(() => {
      const trimmedSearchText = searchText.trim()
      if (!trimmedSearchText) {
        return {
          filteredFeaturedNetworks: featuredNetworks,
          filteredPopularNetworks: popularNetworks
        }
      }
      return {
        filteredFeaturedNetworks: featuredNetworks.filter(({ chainName }) =>
          chainName.toLowerCase().includes(trimmedSearchText)
        ),
        filteredPopularNetworks: popularNetworks.filter(({ chainName }) =>
          chainName.toLowerCase().includes(trimmedSearchText)
        )
      }
    }, [featuredNetworks, popularNetworks, searchText])

  // methods
  const onSubmit = React.useCallback(async () => {
    if (!networks) {
      // wait for networks
      return
    }

    const selectedNetworks: Array<
      Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
    > = []
    const hiddenNets: Array<Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>> =
      []

    for (const net of networks) {
      if (visibleSelectedChainIds.includes(getNetworkId(net))) {
        selectedNetworks.push({ chainId: net.chainId, coin: net.coin })
      } else {
        hiddenNets.push({ chainId: net.chainId, coin: net.coin })
      }
    }

    // hide non-selected networks
    await hideNetworks(hiddenNets).unwrap()

    // force show selected networks
    await restoreNetworks(selectedNetworks).unwrap()

    // Temporary workaround:
    // prevent creation of unwanted bitcoin & filecoin accounts
    dispatch(
      WalletActions.setAllowedNewWalletAccountTypeNetworkIds(
        visibleSelectedChainIds
      )
    )

    history.push(
      onboardingType === 'hardware'
        ? WalletRoutes.OnboardingHardwareWalletCreatePassword
        : onboardingType === 'import'
        ? WalletRoutes.OnboardingRestoreWallet
        : WalletRoutes.OnboardingNewWalletCreatePassword
    )
  }, [
    networks,
    hideNetworks,
    restoreNetworks,
    dispatch,
    visibleSelectedChainIds,
    history,
    onboardingType
  ])

  // effects
  React.useEffect(() => {
    if (!mainnetChainIds.length) {
      return
    }

    // pre-populate selected chains (no testnets)
    setSelectedChainIds(mainnetChainIds)
  }, [mainnetChainIds])

  // computed
  const areAllChainsSelected =
    visibleSelectedChainIds.length ===
    featuredNetworks.length + popularNetworks.length

  // render
  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletSupportedNetworks')}
      subTitle={getLocale('braveWalletChooseChainsToUse')}
      padding='44px 0 0'
    >
      <Column
        fullWidth
        alignItems='flex-start'
      >
        <SearchBar
          value={searchText}
          action={(e) => {
            setSearchText(e.target.value)
          }}
          placeholder='Search networks'
          autoFocus
          isV2
        />
        <Row
          justifyContent='flex-end'
          margin='8px 0'
        >
          <Checkbox
            checked={showTestNets}
            onChange={() => setShowTestNets((prev) => !prev)}
          >
            <Text
              textSize='12px'
              isBold={false}
              color={leo.color.text.primary}
            >
              {getLocale('braveWalletShowTestnets')}
            </Text>
          </Checkbox>
        </Row>

        <ScrollableColumn
          maxHeight={'300px'}
          padding={'0px 8px 0px 0px'}
        >
          <ChainSelectionContextProvider value={selectedChainsContext}>
            {isLoadingNetworks ? (
              <Column
                fullHeight
                fullWidth
              >
                <ProgressRing mode='indeterminate' />
              </Column>
            ) : (
              <>
                {filteredFeaturedNetworks.length > 0 ? (
                  <GroupingText>
                    {getLocale('braveWalletFeatured')}
                  </GroupingText>
                ) : null}

                <NetworkSelectionGrid>
                  {filteredFeaturedNetworks.map((net) => {
                    return (
                      <NetworkCheckbox
                        isDisabled={mandatoryChainIds.includes(net.chainId)}
                        key={getNetworkId(net)}
                        network={net}
                      />
                    )
                  })}
                </NetworkSelectionGrid>

                <Row
                  alignItems='center'
                  justifyContent='space-between'
                >
                  <GroupingText>{getLocale('braveWalletPopular')}</GroupingText>
                  {networks && (
                    <SelectAllText
                      onClick={() => {
                        areAllChainsSelected
                          ? setSelectedChainIds(mandatoryChainIds)
                          : setSelectedChainIds(
                              showTestNets ? networkIds : mainnetChainIds
                            )
                      }}
                    >
                      {getLocale(
                        areAllChainsSelected
                          ? 'braveWalletDeselectAll'
                          : 'braveWalletSelectAll'
                      )}
                    </SelectAllText>
                  )}
                </Row>

                <NetworkSelectionGrid>
                  {filteredPopularNetworks.map((net) => (
                    <NetworkCheckbox
                      key={getNetworkId(net)}
                      network={net}
                    />
                  ))}
                </NetworkSelectionGrid>
              </>
            )}
          </ChainSelectionContextProvider>
        </ScrollableColumn>
      </Column>

      <VerticalSpace space='24px' />

      <NextButtonRow>
        <ContinueButton
          onClick={onSubmit}
          disabled={visibleSelectedChainIds.length === 0}
        >
          {visibleSelectedChainIds.length
            ? getLocale('braveWalletContinueWithXItems')
                .replace(
                  '$1', // Number of items
                  visibleSelectedChainIds.length.toString()
                )
                .replace(
                  '$2', // Item name (maybe plural)
                  visibleSelectedChainIds.length > 1
                    ? getLocale('braveWalletNetworks')
                    : getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')
                )
            : getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </NextButtonRow>
      <MutedLinkText onClick={openNetworkSettings}>
        {getLocale('braveWalletAddNetworksAnytimeInSettings')}
      </MutedLinkText>
    </OnboardingContentLayout>
  )
}
