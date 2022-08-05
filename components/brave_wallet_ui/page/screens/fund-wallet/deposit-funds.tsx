// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import {
  useDispatch,
  useSelector
} from 'react-redux'

// utils
import { getLocale } from '../../../../common/locale'
import { getNetworkInfo, getTokensNetwork } from '../../../utils/network-utils'
import { generateQRCode } from '../../../utils/qr-code-utils'
import { makeNetworkAsset } from '../../../options/asset-options'

// types
import {
  BraveWallet,
  SupportedTestNetworks,
  WalletAccountType,
  WalletRoutes,
  WalletState
} from '../../../constants/types'

// actions
import { WalletActions } from '../../../common/actions'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'

// hooks
import { useHasAccount } from '../../../common/hooks'
import { useIsMounted } from '../../../common/hooks/useIsMounted'

// style
import { Column, Flex, LoadingIcon, Row } from '../../../components/shared/style'
import { Description, MainWrapper, NextButtonRow, StyledWrapper, Title, VerticalSpace } from '../onboarding/onboarding.style'
import {
  QRCodeImage,
  ScrollContainer,
  SearchWrapper,
  SelectAssetWrapper
} from './fund-wallet.style'

// components
import SearchBar from '../../../components/shared/search-bar'
import SelectAccountItem from '../../../components/shared/select-account-item'
import SelectAccount from '../../../components/shared/select-account'
import WalletPageLayout from '../../../components/desktop/wallet-page-layout/index'
import TokenLists from '../../../components/desktop/views/portfolio/components/token-lists'
import { StepsNavigation } from '../../../components/desktop/steps-navigation/steps-navigation'
import { BuyAssetOptionItem } from '../../../components/shared/buy-option/buy-asset-option'
import { NavButton } from '../../../components/extension/buttons/nav-button/index'
import CreateAccountTab from '../../../components/buy-send-swap/create-account'
import SelectHeader from '../../../components/buy-send-swap/select-header'

export const DepositFundsScreen = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const {
    accounts,
    selectedNetworkFilter,
    selectedAccount,
    fullTokenList,
    networkList
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // custom hooks
  const isMounted = useIsMounted()
  const { needsAccount } = useHasAccount()

  // state
  const [showDepositAddress, setShowDepositAddress] = React.useState<boolean>(false)
  const [showAccountSearch, setShowAccountSearch] = React.useState<boolean>(false)
  const [accountSearchText, setAccountSearchText] = React.useState<string>('')
  const [qrCode, setQRCode] = React.useState<string>('')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken | undefined>(undefined)

  // memos
  const isNextStepEnabled = React.useMemo(() => !!selectedAsset, [selectedAsset])

  const mainnetsList = React.useMemo(() =>
    networkList.filter(net => {
      // skip testnet & localhost chains
      return !SupportedTestNetworks.includes(net.chainId)
    }),
    [networkList]
  )

  const mainnetNetworkAssetsList = React.useMemo(() => {
    return mainnetsList
    .map(net => makeNetworkAsset(net))
  }, [networkList])

  const fullAssetsList = React.useMemo(() => {
    // separate BAT from other tokens in the list so they can be placed higher in the list
    const { bat, nonBat } = fullTokenList.reduce((acc, t) => {
      if (
        t.symbol.toLowerCase() === 'bat' ||
        t.symbol.toLowerCase() === 'wbat' || // wormhole BAT
        t.symbol.toLowerCase() === 'bat.e' // Avalanche C-Chain BAT
      ) {
        acc.bat.push(t)
        return acc
      }
      acc.nonBat.push(t)
      return acc
    }, {
      bat: [] as BraveWallet.BlockchainToken[],
      nonBat: [] as BraveWallet.BlockchainToken[]
    })
    return [...mainnetNetworkAssetsList, ...bat, ...nonBat]
  }, [mainnetNetworkAssetsList, fullTokenList])

  const assetsForFilteredNetwork = React.useMemo(() => {
    const assets = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? fullAssetsList
      : fullAssetsList.filter(({ chainId }) => selectedNetworkFilter.chainId === chainId)

    return assets.map(asset => ({ asset, assetBalance: '1' }))
  }, [selectedNetworkFilter.chainId, fullAssetsList])

  const selectedAssetNetwork = React.useMemo(() => {
    return selectedAsset
      ? getNetworkInfo(selectedAsset.chainId, selectedAsset.coin, mainnetsList)
      : undefined
  }, [selectedAsset, mainnetsList])

  const accountsForSelectedAssetNetwork = React.useMemo(() => {
    return selectedAssetNetwork
      ? accounts.filter(a => a.coin === selectedAssetNetwork.coin)
      : []
  }, [selectedAssetNetwork, accounts])

  const accountListSearchResults = React.useMemo(() => {
    if (accountSearchText === '') {
      return accountsForSelectedAssetNetwork
    }

    return accountsForSelectedAssetNetwork.filter((item) => {
      return item.name.toLowerCase().startsWith(accountSearchText.toLowerCase())
    })
  }, [accountSearchText, accountsForSelectedAssetNetwork])

  // memos
  const depositTitleText = React.useMemo(() => {
    const isNativeAsset = (
      selectedAsset?.coin === BraveWallet.CoinType.ETH &&
      !selectedAsset?.isErc20 &&
      !selectedAsset?.isErc721
    )
    const isFil = selectedAsset?.coin === BraveWallet.CoinType.FIL
    const isSolOrSpl = selectedAsset?.coin === BraveWallet.CoinType.SOL
    const isErc = selectedAsset?.isErc20 || selectedAsset?.isErc721

    // EVM native network (gas) assets & Filecoin
    if (isNativeAsset || isFil) {
      return getLocale('braveWalletDepositX').replace('$1', selectedAsset.symbol)
    }

    // ERC-based tokens
    if (isErc) {
      return getLocale('braveWalletDepositErc')
    }

    // Solana assets
    if (isSolOrSpl) {
      return getLocale('braveWalletDepositSolSplTokens')
    }

    return ''
  }, [selectedAsset])

  // methods
  const openAccountSearch = React.useCallback(() => setShowAccountSearch(true), [])
  const closeAccountSearch = React.useCallback(() => setShowAccountSearch(false), [])
  const onSearchTextChanged = React.useCallback((e: React.ChangeEvent<HTMLInputElement>) => setAccountSearchText(e.target.value), [])

  const goToPortfolio = React.useCallback(() => {
    history.push(WalletRoutes.Portfolio)
  }, [history])

  const onSelectAccountFromSearch = React.useCallback((account: WalletAccountType) => () => {
    closeAccountSearch()
    dispatch(WalletActions.selectAccount(account))
  }, [closeAccountSearch])

  const onBack = React.useCallback(() => {
    if (!showDepositAddress && history.length) {
      return history.goBack()
    }

    if (showDepositAddress) {
      // go back to asset selection
      setShowDepositAddress(false)
      return closeAccountSearch()
    }
  }, [showDepositAddress, closeAccountSearch, history])

  const nextStep = React.useCallback(() => {
    if (!isNextStepEnabled || !selectedAssetNetwork) {
      return
    }
    dispatch(WalletActions.selectNetwork(selectedAssetNetwork))
    setShowDepositAddress(true)
  }, [isNextStepEnabled, selectedAssetNetwork])

  // effects
  React.useEffect(() => {
    // fetch selected Account QR Code
    generateQRCode(selectedAccount.address).then(qr => {
      if (isMounted) {
        setQRCode(qr)
      }
    })
  }, [selectedAccount, isMounted])

  React.useEffect(() => {
    // unselect asset on chain filter changed
    if (selectedNetworkFilter) {
      setSelectedAsset(undefined)
    }
  }, [selectedNetworkFilter])

  // sync default selected account with selected asset
  React.useEffect(() => {
    if (
      selectedAsset &&
      selectedAssetNetwork &&
      accountsForSelectedAssetNetwork.length && // asset is selected & account is available
      selectedAccount.coin !== selectedAsset.coin // needs to change accounts to one with correct network
    ) {
      dispatch(WalletActions.selectAccount(accountsForSelectedAssetNetwork[0]))
    }
  }, [
    selectedAsset,
    selectedAssetNetwork,
    accountsForSelectedAssetNetwork,
    selectedAccount
  ])

  React.useEffect(() => {
    // load tokens on mount
    if (!fullTokenList.length) {
      dispatch(WalletActions.getAllTokensList())
    }
  }, [fullTokenList])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          {!showAccountSearch &&
            <StepsNavigation
              goBack={onBack}
              onSkip={goToPortfolio}
              skipButtonText={getLocale('braveWalletButtonClose')}
              steps={[]}
              currentStep=''
            />
          }

          {/* Creates wallet Account if needed */}
          {needsAccount && <CreateAccountTab /> }

          {/* Asset Selection */}
          {!needsAccount && !showDepositAddress &&
            <>
              <SelectAssetWrapper>

                <Title>
                  {getLocale('braveWalletDepositFundsTitle')}
                </Title>

                {fullTokenList.length
                  ? <TokenLists
                    enableScroll
                    maxListHeight='38vh'
                    userAssetList={assetsForFilteredNetwork}
                    networks={mainnetsList}
                    hideAddButton
                    renderToken={({ asset }) => {
                      return <BuyAssetOptionItem
                        isSelected={asset === selectedAsset}
                        key={asset.isErc721
                          ? `${asset.contractAddress}-${asset.symbol}-${asset.chainId}`
                          : `${asset.contractAddress}-${asset.tokenId}-${asset.chainId}`}
                        token={asset}
                        tokenNetwork={getTokensNetwork(mainnetsList, asset)}
                        onClick={setSelectedAsset}
                      />
                    }}
                  />
                  : <Column>
                    <LoadingIcon
                      opacity={1}
                      size='100px'
                      color='interactive05'
                    />
                  </Column>
                }

                <VerticalSpace space='12px' />

              </SelectAssetWrapper>

              <NextButtonRow>
                <NavButton
                  buttonType='primary'
                  text={
                    selectedAsset
                      ? getLocale('braveWalletButtonContinue')
                      : getLocale('braveWalletBuySelectAsset')
                  }
                  onSubmit={nextStep}
                  disabled={!isNextStepEnabled}
                />
              </NextButtonRow>
            </>
          }

          {/* Address display & Account selection/search */}
          {!needsAccount && showDepositAddress &&
            <>
              {!showAccountSearch &&
                <>

                  <Title>{depositTitleText}</Title>

                  {selectedAssetNetwork &&
                    <Description>
                      {
                        getLocale('braveWalletDepositOnlySendOnXNetwork')
                          .replace('$1', selectedAssetNetwork.chainName)
                      }
                    </Description>
                  }

                  <Row
                    justifyContent='space-around'
                    alignItems='center'
                  >
                    <Flex>
                      <SelectAccountItem
                        selectedNetwork={selectedAssetNetwork}
                        account={selectedAccount}
                        onSelectAccount={openAccountSearch}
                        showTooltips
                        fullAddress
                      />
                    </Flex>
                  </Row>

                  <QRCodeImage src={qrCode} />
                </>
              }

              {showAccountSearch &&
                <SearchWrapper>
                  <SelectHeader
                    title={getLocale('braveWalletSelectAccount')}
                    onBack={closeAccountSearch}
                    hasAddButton={false}
                  />
                  <SearchBar
                    placeholder={getLocale('braveWalletSearchAccount')}
                    action={onSearchTextChanged}
                  />
                  <ScrollContainer>
                    <SelectAccount
                      accounts={accountListSearchResults}
                      selectedAccount={selectedAccount}
                      onSelectAccount={onSelectAccountFromSearch}
                    />
                  </ScrollContainer>
                </SearchWrapper>
              }
            </>
          }

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default DepositFundsScreen
