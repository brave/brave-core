// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

// constants
import { BraveWallet } from '../constants/types'

// Components
import {
  ConnectWithSite, //
} from '../components/extension/connect-with-site-panel/connect-with-site-panel'
import { WelcomePanel } from '../components/extension/welcome-panel/index'
import { SignPanel } from '../components/extension/sign-panel/index'
import {
  AllowAddChangeNetworkPanel, //
} from '../components/extension/allow_add_change_network_panel/allow_add_change_network_panel'
import {
  ConnectHardwareWalletPanel, //
} from '../components/extension/connect-hardware-wallet-panel/index'
import {
  AddSuggestedTokenPanel, //
} from '../components/extension/add_suggested_token_panel/add_suggested_token_panel'
import {
  ProvidePubKeyPanel,
  DecryptRequestPanel,
} from '../components/extension/encryption-key-panel/index'

import {
  StyledExtensionWrapper,
  LongWrapper,
  ConnectWithSiteWrapper,
} from '../stories/style'
import { PanelWrapper, WelcomePanelWrapper } from './style'
import { FullScreenWrapper } from '../page/screens/page-screen.styles'

import { TransactionStatus } from '../components/extension/post-confirmation'
import {
  useSafePanelSelector,
  useSafeWalletSelector,
  useUnsafePanelSelector,
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
  useGetPendingSignSolTransactionsRequestsQuery,
  useGetPendingSignCardanoTransactionRequestsQuery,
  useGetPendingTokenSuggestionRequestsQuery,
} from '../common/slices/api.slice'
import { useAccountsQuery } from '../common/slices/api.slice.extra'
import {
  useSelectedPendingTransaction, //
} from '../common/hooks/use-pending-transaction'
import PageContainer from '../page/container'
import {
  SignInWithEthereumError, //
} from '../components/extension/sign_in_with_ethereum/sign_in_with_ethereum_error'
import {
  SignInWithEthereum, //
} from '../components/extension/sign_in_with_ethereum/sign_in_with_ethereum'
import {
  PendingTransactionPanel, //
} from '../components/extension/pending_transaction_panel/pending_transaction_panel'
import {
  PendingSignSolanaTransactionsRequestsPanel, //
} from '../components/extension/pending_sign_solana_txs_requests_panel/pending_sign_solana_txs_requests_panel'
import {
  PendingSignCardanoTransactionRequestsPanel, //
} from '../components/extension/pending_sign_cardano_tx_requests_panel/pending_sign_cardano_tx_requests_panel'

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
    PanelSelectors.hardwareWalletCode,
  )

  // panel selectors (unsafe)
  const selectedTransactionId = useUnsafePanelSelector(
    PanelSelectors.selectedTransactionId,
  )
  const connectToSiteOrigin = useUnsafePanelSelector(
    PanelSelectors.connectToSiteOrigin,
  )
  const connectingAccounts = useUnsafePanelSelector(
    PanelSelectors.connectingAccounts,
  )

  // queries
  const { accounts } = useAccountsQuery()
  const { data: addChainRequest } = useGetPendingAddChainRequestQuery()
  const { data: switchChainRequest } = useGetPendingSwitchChainRequestQuery()
  const { data: decryptRequest } = useGetPendingDecryptRequestQuery()
  const {
    data: getEncryptionPublicKeyRequest,
    isLoading: isLoadingPendingPublicKeyRequest,
  } = useGetPendingGetEncryptionPublicKeyRequestQuery()
  const {
    data: signSolTransactionsRequests,
    isLoading: isLoadingSignSolTransactionsRequests,
  } = useGetPendingSignSolTransactionsRequestsQuery()
  const {
    data: signCardanoTransactionRequests,
    isLoading: isLoadingSignCardanoTransactionRequests,
  } = useGetPendingSignCardanoTransactionRequestsQuery()
  const { data: signMessageData, isLoading: isLoadingSignMessageData } =
    useGetPendingSignMessageRequestsQuery()
  const {
    data: signMessageErrorData,
    isLoading: isLoadingSignMessageErrorData,
  } = useGetPendingSignMessageErrorsQuery()
  const { data: addTokenRequests = [], isLoading: isLoadingAddTokenRequests } =
    useGetPendingTokenSuggestionRequestsQuery()
  const {
    selectedPendingTransaction,
    isLoading: isLoadingPendingTransactions,
  } = useSelectedPendingTransaction()

  // computed
  const isLoadingPendingActions =
    isLoadingPendingTransactions
    || isLoadingPendingPublicKeyRequest
    || isLoadingSignSolTransactionsRequests
    || isLoadingSignCardanoTransactionRequests
    || isLoadingSignMessageData
    || isLoadingSignMessageErrorData
    || isLoadingAddTokenRequests

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
    const accountsToConnect = accounts.filter((account) => {
      if (account.accountId.coin === BraveWallet.CoinType.ADA) {
        return connectingAccounts.includes(account.accountId.uniqueKey)
      } else {
        return connectingAccounts.includes(account.address.toLowerCase())
      }
    })
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
    selectedPanel === 'connectHardwareWallet'
    && (selectedPendingTransaction
      || signMessageData?.length
      || signSolTransactionsRequests?.length)
  ) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <ConnectHardwareWalletPanel hardwareWalletCode={hardwareWalletCode} />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (addChainRequest) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <AllowAddChangeNetworkPanel addChainRequest={addChainRequest} />
      </PanelWrapper>
    )
  }

  if (switchChainRequest) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <AllowAddChangeNetworkPanel switchChainRequest={switchChainRequest} />
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

  if (signMessageData?.length && signMessageData[0].signData.ethSiweData) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <SignInWithEthereum data={signMessageData[0]} />
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
      <PanelWrapper
        width={390}
        height={650}
      >
        <AddSuggestedTokenPanel />
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'transactionStatus' && selectedTransactionId) {
    return (
      <PanelWrapper
        width={390}
        height={650}
      >
        <LongWrapper padding='0px'>
          <TransactionStatus transactionLookup={selectedTransactionId} />
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
        <PendingTransactionPanel
          selectedPendingTransaction={selectedPendingTransaction}
        />
      </PanelWrapper>
    )
  }

  if (signSolTransactionsRequests?.length) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper padding='0px'>
          <PendingSignSolanaTransactionsRequestsPanel />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (signCardanoTransactionRequests?.length) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper padding='0px'>
          <PendingSignCardanoTransactionRequestsPanel />
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
