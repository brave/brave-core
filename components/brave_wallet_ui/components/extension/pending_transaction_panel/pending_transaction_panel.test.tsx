// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'

// components
import {
  WalletPanelTestWrapper //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { PendingTransactionPanel } from './pending_transaction_panel'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { findAccountByAccountId } from '../../../utils/account-utils'
import {
  accountInfoEntityAdaptor //
} from '../../../common/slices/entities/account-info.entity'

// mocks
import {
  mockSolanaTransactionInfo,
  mockSolanaTransactionInfoAccount,
  mockSvmTxInfos,
  mockTransactionInfo //
} from '../../../stories/mock-data/mock-transaction-info'
import {
  mockEvmSimulatedResponse,
  mockSolStakingChangeEvent,
  mockSvmSimulationResult
} from '../../../common/constants/mocks'

const loadingPanelTestId = 'loading-panel'
const confirmTxPanelTestId = 'confirm-tx-panel'

const mockAccountsRegistry = accountInfoEntityAdaptor.addMany(
  accountInfoEntityAdaptor.getInitialState(),
  [mockSolanaTransactionInfoAccount]
)

describe('Pending Transaction Panel', () => {
  test('should display transaction origin after loading data', async () => {
    render(
      <WalletPanelTestWrapper>
        <PendingTransactionPanel
          selectedPendingTransaction={mockTransactionInfo}
        />
      </WalletPanelTestWrapper>
    )

    // loading skeleton should appear first
    expect(screen.getByTestId(loadingPanelTestId)).toBeVisible()

    // wait for loading to finish
    await waitFor(() =>
      expect(screen.queryByTestId(loadingPanelTestId)).not.toBeInTheDocument()
    )

    // then the transaction origin should appear
    expect(screen.getByTestId(confirmTxPanelTestId)).toBeVisible()
    expect(screen.getByText('uniswap.org')).toBeVisible()
    expect(screen.getByText('uniswap.org')).toHaveTextContent('uniswap.org')
  })

  test(
    'should display transaction simulation opt-in screen ' +
      'when the feature flag is enabled but not yet acknowledged',
    async () => {
      render(
        <WalletPanelTestWrapper
          walletApiDataOverrides={{
            simulationOptInStatus: BraveWallet.BlowfishOptInStatus.kUnset
          }}
        >
          <PendingTransactionPanel
            selectedPendingTransaction={mockTransactionInfo}
          />
        </WalletPanelTestWrapper>
      )

      // loading panel should appear first
      expect(screen.getByTestId(loadingPanelTestId)).toBeVisible()

      // wait for loading to finish
      await waitFor(() =>
        expect(screen.queryByTestId(loadingPanelTestId)).not.toBeInTheDocument()
      )

      // tx simulations opt-in screen should be displayed
      expect(
        screen.getByText(getLocale('braveWalletEnableTransactionSimulation'))
      ).toBeVisible()
    }
  )

  describe('EVM Simulations', () => {
    test('should show the retry button if the transaction simulation fails', async () => {
      render(
        <WalletPanelTestWrapper
          walletApiDataOverrides={{
            simulationOptInStatus: BraveWallet.BlowfishOptInStatus.kAllowed,
            evmSimulationResponse: undefined
          }}
        >
          <PendingTransactionPanel
            selectedPendingTransaction={mockTransactionInfo}
          />
        </WalletPanelTestWrapper>
      )

      // loading panel should appear first
      expect(screen.getByTestId(loadingPanelTestId)).toBeVisible()

      // wait for loading to finish
      await waitFor(() =>
        expect(screen.queryByTestId(loadingPanelTestId)).not.toBeInTheDocument()
      )

      // retry button should be displayed
      expect(
        screen.getByText(getLocale('braveWalletButtonRetry'))
      ).toBeVisible()
    })

    test('should show the simulated transaction panel after simulation is successful', async () => {
      render(
        <WalletPanelTestWrapper
          walletApiDataOverrides={{
            simulationOptInStatus: BraveWallet.BlowfishOptInStatus.kAllowed,
            evmSimulationResponse: mockEvmSimulatedResponse
          }}
        >
          <PendingTransactionPanel
            selectedPendingTransaction={mockTransactionInfo}
          />
        </WalletPanelTestWrapper>
      )

      // loading panel should appear first
      expect(screen.getByTestId(loadingPanelTestId)).toBeVisible()

      // wait for loading to finish
      await waitFor(() =>
        expect(screen.queryByTestId(loadingPanelTestId)).not.toBeInTheDocument()
      )

      // retry button should NOT be displayed
      expect(
        screen.queryByText(getLocale('braveWalletButtonRetry'))
      ).not.toBeInTheDocument()

      // expected changes should be displayed
      expect(
        screen.queryByText(getLocale('braveWalletEstimatedBalanceChange'))
      ).toBeInTheDocument()
      const approvalDetailsGrouping = screen.getByText(
        getLocale('braveWalletApprovalDetails')
      )
      expect(approvalDetailsGrouping).toBeInTheDocument()

      // Should display simulation results:
      // Balance changes
      expect(screen.queryByText('- 1 ETH')).toBeInTheDocument()
      expect(screen.queryByText('+ 14.4539 LINK')).toBeInTheDocument()
      expect(screen.getByText('- PudgyPenguins #7238')).toBeInTheDocument()
      expect(screen.getByText('+ BoredApeYachtClub #6603')).toBeInTheDocument()

      // Approvals
      expect(screen.getByText('0.000000000000000001 LINK')).toBeInTheDocument()
      expect(screen.getByText('10 LINK')).toBeInTheDocument()
      expect(screen.getByText('0 AMC')).toBeInTheDocument()
      expect(screen.getByText('1 AMC')).toBeInTheDocument()
    })
  })

  describe('SVM Simulations', () => {
    test('should show the retry button if the transaction simulation fails', async () => {
      render(
        <WalletPanelTestWrapper
          walletApiDataOverrides={{
            simulationOptInStatus: BraveWallet.BlowfishOptInStatus.kAllowed,
            evmSimulationResponse: undefined,
            svmSimulationResponse: undefined
          }}
        >
          <PendingTransactionPanel
            selectedPendingTransaction={mockSolanaTransactionInfo}
          />
        </WalletPanelTestWrapper>
      )

      // loading panel should appear first
      expect(screen.getByTestId(loadingPanelTestId)).toBeVisible()

      // wait for loading to finish
      await waitFor(() =>
        expect(screen.queryByTestId(loadingPanelTestId)).not.toBeInTheDocument()
      )

      // retry button should be displayed
      expect(
        screen.getByText(getLocale('braveWalletButtonRetry'))
      ).toBeVisible()
    })

    test('should show the simulated transaction panel after simulation is successful', async () => {
      render(
        <WalletPanelTestWrapper
          walletApiDataOverrides={{
            simulationOptInStatus: BraveWallet.BlowfishOptInStatus.kAllowed,
            accountInfos: [mockSolanaTransactionInfoAccount],
            svmSimulationResponse: mockSvmSimulationResult,
            selectedAccountId: findAccountByAccountId(
              mockSvmTxInfos[0].fromAccountId,
              mockAccountsRegistry
            )?.accountId,
            transactionInfos: mockSvmTxInfos
          }}
          uiStateOverride={{
            selectedPendingTransactionId: mockSolanaTransactionInfo.id
          }}
        >
          <PendingTransactionPanel
            selectedPendingTransaction={mockSolanaTransactionInfo}
          />
        </WalletPanelTestWrapper>
      )

      // loading panel should appear first
      expect(screen.getByTestId(loadingPanelTestId)).toBeVisible()

      // wait for loading to finish
      await waitFor(() =>
        expect(screen.queryByTestId(loadingPanelTestId)).not.toBeInTheDocument()
      )

      // retry button should NOT be displayed
      expect(
        screen.queryByText(getLocale('braveWalletButtonRetry'))
      ).not.toBeInTheDocument()

      // expected changes should be displayed
      expect(
        screen.queryByText(getLocale('braveWalletEstimatedBalanceChange'))
      ).toBeInTheDocument()
      expect(
        screen.queryByText(getLocale('braveWalletAuthorityChange'))
      ).toBeInTheDocument()

      // Should display simulation results:
      // Balance changes
      expect(screen.queryByText('- 0.0005 SOL')).toBeInTheDocument()
      expect(screen.queryByText('+ 0.0005 SOL')).toBeInTheDocument()
      expect(screen.queryByText('- 0.000000000001 BAT')).toBeInTheDocument()
      expect(screen.queryByText('- The Degen #2314')).toBeInTheDocument()

      // Staking auth changes
      expect(
        screen.queryByText(getLocale('braveWalletStaker'))
      ).toBeInTheDocument()
      expect(
        screen.queryByText(getLocale('braveWalletWithdrawer'))
      ).toBeInTheDocument()
      const oldAddresses = await screen.findAllByText(
        reduceAddress(
          mockSolStakingChangeEvent.rawInfo.data.solStakeAuthorityChangeData!
            .currentAuthorities.staker
        )
      )
      expect(oldAddresses.length).toBe(2)
      const newAddresses = await screen.findAllByText(
        reduceAddress(
          mockSolStakingChangeEvent.rawInfo.data.solStakeAuthorityChangeData!
            .futureAuthorities.staker
        )
      )
      expect(newAddresses.length).toBe(2)

      // warning should be shown
      expect(
        screen.queryByText(
          getLocale('braveWalletSimulationWarningApprovalToEoa')
        )
      ).toBeInTheDocument()
    })
  })
})
