// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

import { useHistory } from 'react-router-dom'

// Components
import {
  ConnectWithSite //
} from '../components/extension/connect-with-site-panel/connect-with-site-panel'
import { WelcomePanel } from '../components/extension/welcome-panel/index'
import { SignPanel } from '../components/extension/sign-panel/index'
import {
  AllowAddChangeNetworkPanel //
} from '../components/extension/allow-add-change-network-panel/index'
import {
  ConfirmTransactionPanel //
} from '../components/extension/confirm-transaction-panel/confirm-transaction-panel'
import {
  ConnectHardwareWalletPanel //
} from '../components/extension/connect-hardware-wallet-panel/index'
import {
  AddSuggestedTokenPanel //
} from '../components/extension/add-suggested-token-panel/index'
import {
  ProvidePubKeyPanel,
  DecryptRequestPanel
} from '../components/extension/encryption-key-panel/index'

import { getInitialSessionRoute } from '../utils/routes-utils'
import {
  StyledExtensionWrapper,
  LongWrapper,
  ConnectWithSiteWrapper
} from '../stories/style'
import { PanelWrapper, WelcomePanelWrapper } from './style'

import { BraveWallet, WalletRoutes } from '../constants/types'

import { SignTransactionPanel } from '../components/extension/sign-panel/sign-transaction-panel'
import { ConfirmSwapTransaction } from '../components/extension/confirm-transaction-panel/swap'
import { TransactionStatus } from '../components/extension/post-confirmation'
import {
  useSafePanelSelector,
  useSafeWalletSelector,
  useUnsafePanelSelector
} from '../common/hooks/use-safe-selector'
import { WalletSelectors } from '../common/selectors'
import { PanelSelectors } from './selectors'
import {
  useGetNetworkQuery,
  useGetPendingTokenSuggestionRequestsQuery
} from '../common/slices/api.slice'
import {
  useAccountsQuery,
  useSelectedAccountQuery
} from '../common/slices/api.slice.extra'
import {
  usePendingTransactions //
} from '../common/hooks/use-pending-transaction'
import PageContainer from '../page/container'
import {
  SignInWithEthereumError //
} from '../components/extension/sign-panel/sign_in_with_ethereum_error'

// Allow BigInts to be stringified
;(BigInt.prototype as any).toJSON = function () {
  return this.toString()
}

const initialSessionRoute =
  getInitialSessionRoute() || WalletRoutes.PortfolioAssets
let hasInitializedRouter = false

function Container() {
  // routing
  const history = useHistory()

  // wallet selectors (safe)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)

  // panel selectors (safe)
  const selectedPanel = useSafePanelSelector(PanelSelectors.selectedPanel)
  const hardwareWalletCode = useSafePanelSelector(
    PanelSelectors.hardwareWalletCode
  )
  const selectedTransactionId = useSafePanelSelector(
    PanelSelectors.selectedTransactionId
  )

  // panel selectors (unsafe)
  const connectToSiteOrigin = useUnsafePanelSelector(
    PanelSelectors.connectToSiteOrigin
  )
  const addChainRequest = useUnsafePanelSelector(PanelSelectors.addChainRequest)
  const signMessageData = useUnsafePanelSelector(PanelSelectors.signMessageData)
  const switchChainRequest = useUnsafePanelSelector(
    PanelSelectors.switchChainRequest
  )
  const getEncryptionPublicKeyRequest = useUnsafePanelSelector(
    PanelSelectors.getEncryptionPublicKeyRequest
  )
  const decryptRequest = useUnsafePanelSelector(PanelSelectors.decryptRequest)
  const connectingAccounts = useUnsafePanelSelector(
    PanelSelectors.connectingAccounts
  )
  const signMessageErrorData = useUnsafePanelSelector(
    PanelSelectors.signMessageErrorData
  )

  const signTransactionRequests = useUnsafePanelSelector(
    PanelSelectors.signTransactionRequests
  )
  const signAllTransactionsRequests = useUnsafePanelSelector(
    PanelSelectors.signAllTransactionsRequests
  )

  // queries & mutations
  const { accounts } = useAccountsQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: switchChainRequestNetwork } = useGetNetworkQuery(
    switchChainRequest.chainId
      ? {
          chainId: switchChainRequest.chainId,
          // Passed ETH here since AllowAddChangeNetworkPanel
          // is only used for EVM networks
          // and switchChainRequest doesn't return coinType.
          coin: BraveWallet.CoinType.ETH
        }
      : skipToken
  )
  const { data: addTokenRequests = [] } =
    useGetPendingTokenSuggestionRequestsQuery()

  // TODO(petemill): If initial data or UI takes a noticeable amount of time to
  // arrive consider rendering a "loading" indicator when `hasInitialized ===
  // false`, and also using `React.lazy` to put all the main UI in a separate JS
  // bundle and display that loading indicator ASAP.
  const { selectedPendingTransaction } = usePendingTransactions()

  const canInitializePageRouter =
    !isWalletLocked &&
    !hasInitializedRouter &&
    hasInitialized &&
    isWalletCreated

  // initialize session route
  React.useEffect(() => {
    if (canInitializePageRouter) {
      history.push(initialSessionRoute)
      hasInitializedRouter = true
    }
  }, [canInitializePageRouter])

  if (!hasInitialized) {
    return null
  }

  if (!isWalletCreated) {
    return (
      <WelcomePanelWrapper>
        <LongWrapper>
          <WelcomePanel />
        </LongWrapper>
      </WelcomePanelWrapper>
    )
  }

  if (isWalletLocked) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <PageContainer />
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'transactionStatus' && selectedTransactionId) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionStatus transactionId={selectedTransactionId} />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (
    selectedAccount &&
    (selectedPendingTransaction ||
      signMessageData.length ||
      signAllTransactionsRequests.length ||
      signTransactionRequests.length) &&
    selectedPanel === 'connectHardwareWallet'
  ) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <ConnectHardwareWalletPanel
            account={selectedAccount}
            hardwareWalletCode={hardwareWalletCode}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (
    selectedPendingTransaction?.txType === BraveWallet.TransactionType.ETHSwap
  ) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ConfirmSwapTransaction />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPendingTransaction) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <LongWrapper>
          <ConfirmTransactionPanel />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (addTokenRequests.length) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <AddSuggestedTokenPanel />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'addEthereumChain') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel
            originInfo={addChainRequest.originInfo}
            networkPayload={addChainRequest.networkInfo}
            panelType='add'
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'switchEthereumChain' && switchChainRequestNetwork) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel
            originInfo={switchChainRequest.originInfo}
            networkPayload={switchChainRequestNetwork}
            panelType='change'
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (signMessageErrorData.length !== 0) {
    return (
      <PanelWrapper>
        <SignInWithEthereumError />
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'signData') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <SignPanel
            signMessageData={signMessageData}
            // Pass a boolean here if the signing method is risky
            showWarning={false}
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (
    (signAllTransactionsRequests.length > 0 ||
      signTransactionRequests.length > 0) &&
    (selectedPanel === 'signTransaction' ||
      selectedPanel === 'signAllTransactions')
  ) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <SignTransactionPanel
            signMode={
              selectedPanel === 'signAllTransactions' ? 'signAllTxs' : 'signTx'
            }
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (
    selectedPanel === 'provideEncryptionKey' &&
    getEncryptionPublicKeyRequest
  ) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ProvidePubKeyPanel payload={getEncryptionPublicKeyRequest} />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'allowReadingEncryptedMessage' && decryptRequest) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <DecryptRequestPanel payload={decryptRequest} />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'connectWithSite') {
    const accountsToConnect = accounts.filter((account) =>
      connectingAccounts.includes(account.address.toLowerCase())
    )
    return (
      <PanelWrapper
        width={390}
        height={600}
      >
        <ConnectWithSiteWrapper>
          <ConnectWithSite
            originInfo={connectToSiteOrigin}
            accountsToConnect={accountsToConnect}
          />
        </ConnectWithSiteWrapper>
      </PanelWrapper>
    )
  }

  return (
    <PanelWrapper
      width={390}
      height={650}
    >
      <PageContainer />
    </PanelWrapper>
  )
}

export default Container
