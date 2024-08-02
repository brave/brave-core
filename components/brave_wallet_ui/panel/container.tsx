// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

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
  ConnectHardwareWalletPanel //
} from '../components/extension/connect-hardware-wallet-panel/index'
import {
  AddSuggestedTokenPanel //
} from '../components/extension/add-suggested-token-panel/index'
import {
  ProvidePubKeyPanel,
  DecryptRequestPanel
} from '../components/extension/encryption-key-panel/index'

import {
  StyledExtensionWrapper,
  LongWrapper,
  ConnectWithSiteWrapper
} from '../stories/style'
import { PanelWrapper, WelcomePanelWrapper } from './style'
import { FullScreenWrapper } from '../page/screens/page-screen.styles'

import { TransactionStatus } from '../components/extension/post-confirmation'
import {
  useSafePanelSelector,
  useSafeWalletSelector,
  useUnsafePanelSelector
} from '../common/hooks/use-safe-selector'
import { WalletSelectors } from '../common/selectors'
import { PanelSelectors } from './selectors'
import {
  useGetPendingAddChainRequestQuery,
  useGetPendingDecryptRequestQuery,
  useGetPendingGetEncryptionPublicKeyRequestQuery,
  useGetPendingSignMessageErrorsQuery,
  useGetPendingSignMessageRequestsQuery,
  useGetPendingSwitchChainRequestQuery,
  useGetPendingSignAllTransactionsRequestsQuery,
  useGetPendingTokenSuggestionRequestsQuery,
  useGetPendingSignTransactionRequestsQuery
} from '../common/slices/api.slice'
import {
  useAccountsQuery,
  useSelectedAccountQuery
} from '../common/slices/api.slice.extra'
import {
  useSelectedPendingTransaction //
} from '../common/hooks/use-pending-transaction'
import PageContainer from '../page/container'
import {
  SignInWithEthereumError //
} from '../components/extension/sign-panel/sign_in_with_ethereum_error'
import {
  PendingTransactionPanel //
} from '../components/extension/pending_transaction_panel/pending_transaction_panel'
import {
  PendingSignatureRequestsPanel //
} from '../components/extension/pending_signature_requests_panel/pending_signature_requests_panel'

// Allow BigInts to be stringified
;(BigInt.prototype as any).toJSON = function () {
  return this.toString()
}

function Container() {
  // wallet selectors (safe)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)

  // panel selectors (safe)
  const selectedPanel = useSafePanelSelector(PanelSelectors.selectedPanel)
  const hardwareWalletCode = useSafePanelSelector(
    PanelSelectors.hardwareWalletCode
  )

  // panel selectors (unsafe)
  const selectedTransactionId = useUnsafePanelSelector(
    PanelSelectors.selectedTransactionId
  )
  const connectToSiteOrigin = useUnsafePanelSelector(
    PanelSelectors.connectToSiteOrigin
  )
  const connectingAccounts = useUnsafePanelSelector(
    PanelSelectors.connectingAccounts
  )

  // queries
  const { accounts } = useAccountsQuery()
  const { data: selectedAccount } = useSelectedAccountQuery()
  const { data: addChainRequest } = useGetPendingAddChainRequestQuery()
  const { data: switchChainRequest } = useGetPendingSwitchChainRequestQuery()
  const { data: decryptRequest } = useGetPendingDecryptRequestQuery()
  const {
    data: getEncryptionPublicKeyRequest,
    isLoading: isLoadingPendingPublicKeyRequest
  } = useGetPendingGetEncryptionPublicKeyRequestQuery()
  const {
    data: signTransactionRequests,
    isLoading: isLoadingSignTransactionRequests
  } = useGetPendingSignTransactionRequestsQuery()
  const {
    data: signAllTransactionsRequests,
    isLoading: isLoadingSignAllTransactionsRequests
  } = useGetPendingSignAllTransactionsRequestsQuery()
  const { data: signMessageData, isLoading: isLoadingSignMessageData } =
    useGetPendingSignMessageRequestsQuery()
  const {
    data: signMessageErrorData,
    isLoading: isLoadingSignMessageErrorData
  } = useGetPendingSignMessageErrorsQuery()
  const { data: addTokenRequests = [], isLoading: isLoadingAddTokenRequests } =
    useGetPendingTokenSuggestionRequestsQuery()
  const {
    selectedPendingTransaction,
    isLoading: isLoadingPendingTransactions
  } = useSelectedPendingTransaction()

  // computed
  const isLoadingPendingActions =
    isLoadingPendingTransactions ||
    isLoadingPendingPublicKeyRequest ||
    isLoadingSignTransactionRequests ||
    isLoadingSignAllTransactionsRequests ||
    isLoadingSignMessageData ||
    isLoadingSignMessageErrorData ||
    isLoadingAddTokenRequests

  // render
  if (!hasInitialized || isLoadingPendingActions) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <FullScreenWrapper>
          <ProgressRing mode='indeterminate' />
        </FullScreenWrapper>
      </PanelWrapper>
    )
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

  if (
    selectedPanel === 'connectHardwareWallet' &&
    selectedAccount &&
    (selectedPendingTransaction ||
      signMessageData?.length ||
      signAllTransactionsRequests?.length ||
      signTransactionRequests?.length)
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

  if (addChainRequest) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel addChainRequest={addChainRequest} />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (switchChainRequest) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel switchChainRequest={switchChainRequest} />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (signMessageErrorData?.length) {
    return (
      <PanelWrapper>
        <SignInWithEthereumError />
      </PanelWrapper>
    )
  }

  if (getEncryptionPublicKeyRequest) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ProvidePubKeyPanel payload={getEncryptionPublicKeyRequest} />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (decryptRequest) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <DecryptRequestPanel payload={decryptRequest} />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (signMessageData?.length) {
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

  if (addTokenRequests.length) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <AddSuggestedTokenPanel />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'transactionStatus' && selectedTransactionId) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionStatus transactionLookup={selectedTransactionId} />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPendingTransaction) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <LongWrapper padding='0px'>
          <PendingTransactionPanel
            selectedPendingTransaction={selectedPendingTransaction}
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (signAllTransactionsRequests?.length || signTransactionRequests?.length) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper padding='0px'>
          <PendingSignatureRequestsPanel
            signMode={
              signAllTransactionsRequests?.length ? 'signAllTxs' : 'signTx'
            }
          />
        </LongWrapper>
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
