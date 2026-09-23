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
} from '../components/extension/connect_hardware_wallet_panel/connect_hardware_wallet_panel'
import {
  AddSuggestedTokenPanel, //
} from '../components/extension/add_suggested_token_panel/add_suggested_token_panel'
import {
  ProvidePublicEncryptionKeyPanel, //
} from '../components/extension/public_encryption_key_panels/provide_public_encryption_key_panel'
import {
  DecryptMessageRequestPanel, //
} from '../components/extension/public_encryption_key_panels/decrypt_message_request_panel'
import { ConnectWithSiteWrapper } from '../stories/style'
import { PanelWrapper } from './panel_wrapper/panel_wrapper'
import { FullScreenWrapper } from '../page/screens/page-screen.styles'

import { TransactionStatus } from '../components/extension/post-confirmation'
import {
  useSafePanelSelector,
  useSafeUISelector,
  useSafeWalletSelector,
  useUnsafePanelSelector,
  useUnsafeUISelector,
} from '../common/hooks/use-safe-selector'
import { UISelectors, WalletSelectors } from '../common/selectors'
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
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)

  // panel selectors (safe)
  const selectedPanel = useSafePanelSelector(PanelSelectors.selectedPanel)
  const hardwareWalletCode = useSafePanelSelector(
    PanelSelectors.hardwareWalletCode,
  )

  // panel selectors (unsafe)
  const connectToSiteOrigin = useUnsafePanelSelector(
    PanelSelectors.connectToSiteOrigin,
  )
  const connectingAccounts = useUnsafePanelSelector(
    PanelSelectors.connectingAccounts,
  )

  // ui selectors (unsafe) — shared confirm/status state with Desktop page
  const selectedTransactionId = useUnsafeUISelector(
    UISelectors.selectedTransactionId,
  )
  const submittingTransaction = useUnsafeUISelector(
    UISelectors.submittingTransaction,
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

  const pendingOrConfirmingTransaction =
    selectedPendingTransaction ?? submittingTransaction

  // render
  if (!hasInitialized || (isLoadingPendingActions && !isSidePanel)) {
    return (
      <PanelWrapper>
        <FullScreenWrapper>
          <ProgressRing mode='indeterminate' />
        </FullScreenWrapper>
      </PanelWrapper>
    )
  }

  if (!isWalletCreated) {
    return (
      <PanelWrapper>
        <WelcomePanel />
      </PanelWrapper>
    )
  }

  if (isWalletLocked) {
    return (
      <PanelWrapper>
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
      <PanelWrapper>
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
      <PanelWrapper>
        <ConnectHardwareWalletPanel hardwareWalletCode={hardwareWalletCode} />
      </PanelWrapper>
    )
  }

  if (addChainRequest) {
    return (
      <PanelWrapper>
        <AllowAddChangeNetworkPanel addChainRequest={addChainRequest} />
      </PanelWrapper>
    )
  }

  if (switchChainRequest) {
    return (
      <PanelWrapper>
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
      <PanelWrapper>
        <SignInWithEthereum data={signMessageData[0]} />
      </PanelWrapper>
    )
  }

  if (getEncryptionPublicKeyRequest) {
    return (
      <PanelWrapper>
        <ProvidePublicEncryptionKeyPanel
          payload={getEncryptionPublicKeyRequest}
        />
      </PanelWrapper>
    )
  }

  if (decryptRequest) {
    return (
      <PanelWrapper>
        <DecryptMessageRequestPanel payload={decryptRequest} />
      </PanelWrapper>
    )
  }

  if (signMessageData?.length) {
    return (
      <PanelWrapper>
        <SignPanel
          signMessageData={signMessageData}
          // Pass a boolean here if the signing method is risky
          showWarning={false}
        />
      </PanelWrapper>
    )
  }

  if (addTokenRequests.length) {
    return (
      <PanelWrapper>
        <AddSuggestedTokenPanel />
      </PanelWrapper>
    )
  }

  if (
    selectedPanel === 'transactionStatus'
    && selectedTransactionId
    && !submittingTransaction
    && !isSidePanel
  ) {
    return (
      <PanelWrapper>
        <TransactionStatus transactionLookup={selectedTransactionId} />
      </PanelWrapper>
    )
  }

  if (pendingOrConfirmingTransaction && !isSidePanel) {
    return (
      <PanelWrapper>
        <PendingTransactionPanel
          selectedPendingTransaction={pendingOrConfirmingTransaction}
        />
      </PanelWrapper>
    )
  }

  if (signSolTransactionsRequests?.length) {
    return (
      <PanelWrapper>
        <PendingSignSolanaTransactionsRequestsPanel />
      </PanelWrapper>
    )
  }

  if (signCardanoTransactionRequests?.length) {
    return (
      <PanelWrapper>
        <PendingSignCardanoTransactionRequestsPanel />
      </PanelWrapper>
    )
  }

  return (
    <PanelWrapper>
      <PageContainer />
    </PanelWrapper>
  )
}

export default Container
