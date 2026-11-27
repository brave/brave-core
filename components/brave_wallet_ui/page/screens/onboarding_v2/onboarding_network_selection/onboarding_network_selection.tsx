// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/nala/react/checkbox'
import ProgressRing from '@brave/nala/react/progressRing'
import { EntityId } from '@reduxjs/toolkit'
import { useHistory } from 'react-router'

// Actions
import { WalletActions } from '$wallet/common/actions'

// Redux
import { useAppDispatch } from '$wallet/common/hooks/use-redux'

// Context
import {
  ChainSelectionContextProvider,
  useChainSelectionContext,
} from '$wallet/common/context/network_selection_context'

// Utils
import { getLocale } from '$web-common/locale'
import {
  getNetworkId, //
} from '$wallet/common/slices/entities/network.entity'
import {
  getOnboardingTypeFromPath,
  openNetworkSettings,
} from '$wallet/utils/routes-utils'
import { useLocationPathName } from '$wallet/common/hooks/use-pathname'

// Queries
import {
  useGetNetworksQuery,
  useHideNetworksMutation,
  useRestoreNetworksMutation,
} from '$wallet/common/slices/api.slice'

// Types
import {
  BraveWallet,
  SupportedTestNetworks,
  WalletRoutes,
} from '$wallet/constants/types'
import {
  SupportedTopLevelChainIds, //
} from '$wallet/options/network-filter-options'

// Components
import { SearchBar } from '$wallet/components/shared/search-bar'
import {
  CreateNetworkIcon, //
} from '$wallet/components/shared/create-network-icon'
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'

// Styled Components
import {
  Column,
  MutedLinkText,
  Row,
  Text,
} from '$wallet/components/shared/style'
import {
  SelectAllText,
  GroupingText,
  NetworkName,
  NetworkSelectionContainer,
  NetworkSelectionGrid,
  ScrollableColumn,
  Section,
  SectionHeader,
} from './onboarding_network_selection.style'
import { Button } from '../onboarding.style'

/** Forcing ETH, SOL account creation until their networks can be hidden */
const mandatoryChainIds = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID,
]

function NetworkCheckbox({
  network,
  isDisabled,
}: {
  network: BraveWallet.NetworkInfo
  isDisabled?: boolean
}) {
  // Context
  const [selectedChains, selectChains] = useChainSelectionContext()

  // Computed
  const chainIdentifier = getNetworkId(network)
  const isChecked = selectedChains.includes(chainIdentifier)

  // Methods
  const onChange = React.useCallback(
    () =>
      selectChains((prev) =>
        isChecked
          ? prev.filter((id) => id !== chainIdentifier)
          : prev.concat(chainIdentifier),
      ),
    [isChecked, selectChains, chainIdentifier],
  )

  // Render
  return (
    <NetworkSelectionContainer
      disabled={isDisabled}
      onClick={onChange}
      type='button'
    >
      <Checkbox
        isDisabled={isDisabled}
        checked={isChecked}
      />
      <CreateNetworkIcon
        network={network}
        marginRight={0}
        size='large'
      />
      <NetworkName>{network.chainName}</NetworkName>
    </NetworkSelectionContainer>
  )
}

export const OnboardingNetworkSelection = () => {
  // Routing
  const history = useHistory()
  const path = useLocationPathName()
  const onboardingType = getOnboardingTypeFromPath(path)

  // Redux
  const dispatch = useAppDispatch()

  // State
  const [searchText, setSearchText] = React.useState('')
  const [selectedChainIds, setSelectedChainIds] = React.useState<EntityId[]>([])

  // Queries
  const { data: networks = [], isLoading: isLoadingNetworks } =
    useGetNetworksQuery()

  // Mutations
  const [hideNetworks] = useHideNetworksMutation()
  const [restoreNetworks] = useRestoreNetworksMutation()

  // Memos
  const selectedChainsContext = React.useMemo(
    () => [selectedChainIds, setSelectedChainIds] as const,
    [selectedChainIds, setSelectedChainIds],
  )

  const mainnetChainIds = React.useMemo(
    () =>
      networks
        .filter((net) => !SupportedTestNetworks.includes(net.chainId))
        .map(getNetworkId),
    [networks],
  )

  const { primaryNetworks, secondaryNetworks, testNetworks } =
    React.useMemo(() => {
      const byChainId = new Map(networks.map((net) => [net.chainId, net]))
      const pick = (chainIds: string[]) =>
        chainIds
          .map((id) => byChainId.get(id))
          .filter((net): net is BraveWallet.NetworkInfo => Boolean(net))

      const secondaryNetworks = networks
        .filter(
          (net) =>
            !SupportedTopLevelChainIds.includes(net.chainId)
            && !SupportedTestNetworks.includes(net.chainId),
        )
        .sort((a, b) => a.chainName.localeCompare(b.chainName))

      return {
        primaryNetworks: pick(SupportedTopLevelChainIds),
        secondaryNetworks,
        testNetworks: pick(SupportedTestNetworks),
      }
    }, [networks])

  const {
    filteredPrimaryNetworks,
    filteredSecondaryNetworks,
    filteredTestNetworks,
  } = React.useMemo(() => {
    const query = searchText.trim().toLowerCase()
    const matches = (net: BraveWallet.NetworkInfo) =>
      !query || net.chainName.toLowerCase().includes(query)
    return {
      filteredPrimaryNetworks: primaryNetworks.filter(matches),
      filteredSecondaryNetworks: secondaryNetworks.filter(matches),
      filteredTestNetworks: testNetworks.filter(matches),
    }
  }, [primaryNetworks, secondaryNetworks, testNetworks, searchText])

  // Methods
  const onSubmit = React.useCallback(async () => {
    const selectedNetworks: Array<
      Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
    > = []
    const hiddenNets: Array<Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>> =
      []

    for (const net of networks) {
      if (selectedChainIds.includes(getNetworkId(net))) {
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
      WalletActions.setAllowedNewWalletAccountTypeNetworkIds(selectedChainIds),
    )

    let nextRoute = WalletRoutes.OnboardingNewWalletSelectRecoveryPhraseLength
    if (onboardingType === 'hardware') {
      nextRoute = WalletRoutes.OnboardingHardwareWalletCreatePassword
    } else if (onboardingType === 'import') {
      nextRoute = WalletRoutes.OnboardingRestoreWallet
    }
    history.push(nextRoute)
  }, [
    networks,
    hideNetworks,
    restoreNetworks,
    dispatch,
    selectedChainIds,
    history,
    onboardingType,
  ])

  // Effects
  React.useEffect(() => {
    if (!mainnetChainIds.length) {
      return
    }

    // pre-populate selected chains (no testnets)
    setSelectedChainIds(mainnetChainIds)
  }, [mainnetChainIds])

  // Computed
  const renderNetworkSection = (
    title: string,
    sectionNetworks: BraveWallet.NetworkInfo[],
  ) => {
    if (!sectionNetworks.length) {
      return null
    }

    const sectionChainIds = sectionNetworks.map(getNetworkId)
    const selectableChainIds: EntityId[] = sectionNetworks
      .filter((net) => !mandatoryChainIds.includes(net.chainId))
      .map(getNetworkId)
    const areAllSectionChainsSelected = sectionChainIds.every((id) =>
      selectedChainIds.includes(id),
    )

    return (
      <Section key={title}>
        <SectionHeader>
          <GroupingText>{title}</GroupingText>
          {selectableChainIds.length > 0 && (
            <SelectAllText
              type='button'
              onClick={(event) => {
                // A click leaves the button focused, which kept it visible
                // after the pointer moved to another section.
                event.currentTarget.blur()
                setSelectedChainIds((prev) =>
                  areAllSectionChainsSelected
                    ? prev.filter((id) => !selectableChainIds.includes(id))
                    : Array.from(new Set([...prev, ...sectionChainIds])),
                )
              }}
            >
              {getLocale(
                areAllSectionChainsSelected
                  ? S.BRAVE_WALLET_DESELECT_ALL
                  : S.BRAVE_WALLET_SELECT_ALL,
              )}
            </SelectAllText>
          )}
        </SectionHeader>
        <NetworkSelectionGrid>
          {sectionNetworks.map((net) => (
            <NetworkCheckbox
              isDisabled={mandatoryChainIds.includes(net.chainId)}
              key={getNetworkId(net)}
              network={net}
            />
          ))}
        </NetworkSelectionGrid>
      </Section>
    )
  }

  const networkLabel =
    selectedChainIds.length > 1
      ? getLocale(S.BRAVE_WALLET_NETWORKS)
      : getLocale(S.BRAVE_WALLET_ALLOW_ADD_NETWORK_PANEL_TITLE)
  const continueButtonLabel = selectedChainIds.length
    ? getLocale(S.BRAVE_WALLET_CONTINUE_WITH_X_ITEMS)
        .replace('$1', selectedChainIds.length.toString())
        .replace('$2', networkLabel)
    : getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)

  // Render
  return (
    <OnboardingContentLayout>
      <Column
        width='100%'
        gap='4px'
        margin='0 0 32px 0'
      >
        <Text
          variant='heading.display3'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_SUPPORTED_NETWORKS)}
        </Text>
        <Text
          variant='large.regular'
          textColor='secondary'
        >
          {getLocale(S.BRAVE_WALLET_CHOOSE_CHAINS_TO_USE)}
        </Text>
      </Column>
      <Column
        fullWidth
        alignItems='flex-start'
        margin='0 0 24px 0'
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
        <ScrollableColumn
          maxHeight='420px'
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
                {renderNetworkSection(
                  getLocale(S.BRAVE_WALLET_PRIMARY_NETWORKS),
                  filteredPrimaryNetworks,
                )}
                {renderNetworkSection(
                  getLocale(S.BRAVE_WALLET_NETWORK_FILTER_SECONDARY),
                  filteredSecondaryNetworks,
                )}
                {renderNetworkSection(
                  getLocale(S.BRAVE_WALLET_NETWORK_FILTER_TEST_NETWORKS),
                  filteredTestNetworks,
                )}
              </>
            )}
          </ChainSelectionContextProvider>
        </ScrollableColumn>
      </Column>

      <Column
        width='100%'
        gap='16px'
      >
        <Row>
          <Button
            onClick={onSubmit}
            disabled={selectedChainIds.length === 0}
          >
            {continueButtonLabel}
          </Button>
        </Row>
        <MutedLinkText onClick={openNetworkSettings}>
          {getLocale(S.BRAVE_WALLET_ADD_NETWORKS_ANYTIME_IN_SETTINGS)}
        </MutedLinkText>
      </Column>
    </OnboardingContentLayout>
  )
}
