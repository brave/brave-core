// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import * as React from 'react'
import { act, render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiDataOverrides } from '../../../constants/testing_types'

// utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../utils/test-utils'
import {
  deserializeTransaction, //
} from '../../../utils/model-serialization-utils'
import getAPIProxy from '../../../common/async/bridge'

// mocks
import {
  mockEthAccount,
  mockSolanaAccount,
  mockSolanaMainnetNetwork,
} from '../../../common/constants/mocks'
import {
  mockETHNativeTokenSendTransaction,
  mockSolanaTransactionInfo,
} from '../../../stories/mock-data/mock-transaction-info'
import { mockEthMainnet } from '../../../stories/mock-data/mock-networks'
import {
  mockSignMessageRequest, //
} from '../../../stories/mock-data/mock-eth-requests'

// components
import { ConnectHardwareWalletPanel } from './connect_hardware_wallet_panel'

// The real module talks to a Trezor/Ledger bridge iframe, which cannot be
// reached from jsdom. Every export is stubbed so the importing endpoint
// modules still resolve their named imports.
jest.mock('../../../common/async/hardware', () => ({
  dialogErrorFromLedgerErrorCode: jest.fn(),
  dialogErrorFromTrezorErrorCode: jest.fn(),
  signTrezorTransaction: jest.fn(),
  signLedgerEthereumTransaction: jest.fn(),
  signLedgerFilecoinTransaction: jest.fn(),
  signLedgerSolanaTransaction: jest.fn(),
  signLedgerBitcoinTransaction: jest.fn(),
  signEthMessageWithHardwareKeyring: jest.fn(),
  signSolTransactionWithHardwareKeyring: jest.fn(),
  cancelHardwareOperation: jest.fn(),
  getDeviceNameFromDevice: jest.fn(),
  loadAccountsFromDevice: jest.fn(),
}))

const hardwareMock = jest.requireMock(
  '../../../common/async/hardware',
) as Record<string, jest.Mock>

// The subset of the mocked proxy this test reaches into. The proxy itself is
// auto-mocked by jest's moduleNameMapper ('/bridge$' -> __mocks__/bridge.ts).
type MockedProxy = {
  braveWalletService: {
    getPendingSignMessageRequests: () => Promise<{
      requests: BraveWallet.SignMessageRequest[]
    }>
  }
  txService: {
    approveTransaction?: jest.Mock
  }
}

// Account H: the Trezor-backed Ethereum account the panel is opened for.
const trezorAccountH: BraveWallet.AccountInfo = {
  ...mockEthAccount,
  name: 'Trezor Account H',
  accountId: {
    ...mockEthAccount.accountId,
    kind: BraveWallet.AccountKind.kHardware,
  },
  hardware: {
    deviceId: 'mock-trezor-device-id',
    path: "m/44'/60'/0'/0/0",
    vendor: BraveWallet.HardwareVendor.kTrezor,
  },
}

// Account L: a Ledger-backed Ethereum account, used for the positive
// control. Ledger is deliberate -- approveHardwareTransaction reaches
// signLedgerEthereumTransaction (stubbed above) directly, whereas the
// Trezor branch first calls getTrezorHardwareKeyring() from
// common/api/hardware_keyrings, which is not mocked here.
const ledgerAccountL: BraveWallet.AccountInfo = {
  ...mockEthAccount,
  name: 'Ledger Account L',
  address: '0x1111111111111111111111111111111111111111',
  accountId: {
    ...mockEthAccount.accountId,
    kind: BraveWallet.AccountKind.kHardware,
    address: '0x1111111111111111111111111111111111111111',
    uniqueKey: 'unique_key_ledger_account_L',
  },
  hardware: {
    deviceId: 'mock-ledger-device-id',
    path: "m/44'/60'/0'/0/0",
    vendor: BraveWallet.HardwareVendor.kLedger,
  },
}

// Account S: a hot software Solana account, unrelated to H.
const softwareAccountS = mockSolanaAccount

// S's pending dApp transaction, awaiting approval.
const pendingSolanaTxForS = deserializeTransaction({
  ...mockSolanaTransactionInfo,
  id: 'sol-tx-for-S',
  txStatus: BraveWallet.TransactionStatus.Unapproved,
})

// L's pending hardware transaction -- the case that *should* be retried.
const pendingEthTxForL = deserializeTransaction({
  ...mockETHNativeTokenSendTransaction,
  id: 'eth-tx-for-L',
  fromAccountId: ledgerAccountL.accountId,
  txStatus: BraveWallet.TransactionStatus.Unapproved,
})

// H's pending personal_sign request. Spread from the shared mock so the
// SignMessageRequest shape stays correct; note that the shared mock uses
// id -1, which the panel treats as "no request", so id must be overridden.
const signMessageRequestForH: BraveWallet.SignMessageRequest = {
  ...mockSignMessageRequest,
  id: 1,
  accountId: trezorAccountH.accountId,
}

/**
 * Renders the panel with a disconnected hardware device, which is the state
 * that keeps the retry loop ticking (hardwareWalletCode 'deviceNotConnected'
 * makes isConnected false and hasResult true).
 *
 * `selectedPendingTransaction` stands in for what ui.slice's
 * newUnapprovedTxAdded matcher does: it claims selectedPendingTransactionId
 * for whichever transaction arrives first, with no regard for which panel is
 * showing or which account it belongs to.
 *
 * Returns a spy standing in for txService.approveTransaction -- the call
 * that signs and broadcasts a *software* account's transaction with no
 * further user interaction.
 */
const renderPanel = ({
  signMessageRequests,
  selectedPendingTransaction,
}: {
  signMessageRequests: BraveWallet.SignMessageRequest[]
  selectedPendingTransaction: BraveWallet.TransactionInfo
}) => {
  const apiOverrides: WalletApiDataOverrides = {
    transactionInfos: [selectedPendingTransaction],
    accountInfos: [trezorAccountH, ledgerAccountL, softwareAccountS],
    networks: [mockEthMainnet, mockSolanaMainnetNetwork],
    // The mock proxy's keyringService.getAllAccounts() asserts that
    // selectedAccountId resolves against accountInfos. It defaults to
    // mockAccount, which the accountInfos override above removes, so it has
    // to be repointed at an account that is actually present.
    selectedAccountId: trezorAccountH.accountId,
  }

  const store = createMockStore(
    {
      uiStateOverride: {
        selectedPendingTransactionId: selectedPendingTransaction.id,
      },
    },
    apiOverrides,
  )

  // Reached for after store creation so createMockStore's resetAPIProxy()
  // does not discard these overrides.
  const proxy = getAPIProxy() as unknown as MockedProxy

  proxy.braveWalletService.getPendingSignMessageRequests = async () => ({
    requests: signMessageRequests,
  })

  const approveTransactionSpy = jest.fn(async () => ({
    status: true,
    errorUnion: {} as BraveWallet.ProviderErrorUnion,
    errorMessage: '',
  }))
  proxy.txService.approveTransaction = approveTransactionSpy

  render(
    <MemoryRouter initialEntries={['/']}>
      <Provider store={store}>
        <WalletTestThemeProvider>
          <ConnectHardwareWalletPanel hardwareWalletCode='deviceNotConnected' />
        </WalletTestThemeProvider>
      </Provider>
    </MemoryRouter>,
  )

  return { approveTransactionSpy }
}

// Covers the disconnected-device retry period (3000ms) with margin.
const advancePastRetryInterval = async () => {
  await act(async () => {
    jest.advanceTimersByTime(4000)
  })
}

const waitForPanelToRender = async () => {
  await waitFor(() => {
    expect(
      screen.getByText(S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_CONNECT),
    ).toBeInTheDocument()
  })
}

describe('ConnectHardwareWalletPanel retry loop', () => {
  beforeEach(() => {
    jest.useFakeTimers()

    // jest.config sets resetMocks, so implementations are re-applied here
    // rather than in the jest.mock factory.
    hardwareMock.signEthMessageWithHardwareKeyring.mockResolvedValue({
      success: false,
      error: 'device disconnected',
      code: 'Method_Interrupted',
    })
    hardwareMock.signLedgerEthereumTransaction.mockResolvedValue({
      success: false,
      error: 'device disconnected',
      code: 'TransportOpenUserCancelled',
    })
    hardwareMock.dialogErrorFromTrezorErrorCode.mockReturnValue(
      'deviceNotConnected',
    )
    hardwareMock.dialogErrorFromLedgerErrorCode.mockReturnValue(
      'deviceNotConnected',
    )
    hardwareMock.getDeviceNameFromDevice.mockResolvedValue({
      success: false,
      deviceName: '',
    })
  })

  afterEach(() => {
    jest.useRealTimers()
  })

  it(
    'does not approve an unrelated software transaction while a hardware'
      + ' sign-message request is displayed',
    async () => {
      const { approveTransactionSpy } = renderPanel({
        signMessageRequests: [signMessageRequestForH],
        selectedPendingTransaction: pendingSolanaTxForS,
      })

      // The panel is showing H's "connect your device" prompt. This is the
      // only request the user can see.
      await waitForPanelToRender()
      await advancePastRetryInterval()

      // Witness: the retry loop really did tick, so the assertion below is
      // meaningful rather than vacuous.
      expect(hardwareMock.signEthMessageWithHardwareKeyring).toHaveBeenCalled()

      // S's transaction was never shown to the user, so the retry loop must
      // not have approved it.
      expect(approveTransactionSpy).not.toHaveBeenCalled()
    },
  )

  it(
    'does not approve a software transaction when no sign-message request'
      + ' is pending',
    async () => {
      // Same panel, same disconnected device, but nothing is being signed --
      // so an `if (isSigning) { ...; return }` early return does not cover
      // this path. The panel is still a hardware-retry surface and has no
      // business approving a software account's transaction.
      const { approveTransactionSpy } = renderPanel({
        signMessageRequests: [],
        selectedPendingTransaction: pendingSolanaTxForS,
      })

      await waitForPanelToRender()
      await advancePastRetryInterval()

      expect(approveTransactionSpy).not.toHaveBeenCalled()
    },
  )

  it('still retries a pending hardware transaction', async () => {
    // Positive control. Without this, narrowing retryHardwareOperation far
    // enough to do nothing at all would satisfy both tests above.
    renderPanel({
      signMessageRequests: [],
      selectedPendingTransaction: pendingEthTxForL,
    })

    await waitForPanelToRender()
    await advancePastRetryInterval()

    expect(hardwareMock.signLedgerEthereumTransaction).toHaveBeenCalled()
  })
})
