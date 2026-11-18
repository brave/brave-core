// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

import {
  BraveWallet,
  emptyProviderErrorCodeUnion,
  HardwareWalletOperation,
} from '$wallet/constants/types'
import { mockAccount, mockEthAccount } from '$wallet/common/constants/mocks'
import { mockSignMessageRequest } from '$wallet/stories/mock-data/mock-eth-requests'
import { mockTransactionInfo } from '$wallet/stories/mock-data/mock-transaction-info'
import { deserializeTransaction } from '$wallet/utils/model-serialization-utils'
import {
  createMockStore,
  WalletTestThemeProvider,
} from '$wallet/utils/test-utils'
import { walletApiReducerPath } from '$wallet/common/slices/api.slice'
import getAPIProxy from '../../../common/async/bridge'
import {
  getDeviceNameFromDevice,
  signEthMessageWithHardwareKeyring,
} from '$wallet/common/async/hardware'
import { ConnectHardwareWalletPanel } from './connect_hardware_wallet_panel'

jest.mock('$wallet/common/async/hardware', () => {
  const actual = jest.requireActual('$wallet/common/async/hardware')
  return {
    ...actual,
    signEthMessageWithHardwareKeyring: jest.fn(),
    getDeviceNameFromDevice: jest.fn(),
  }
})

const hardwareEthAccount: BraveWallet.AccountInfo = {
  ...mockEthAccount,
  name: 'Hardware ETH',
  accountId: {
    ...mockEthAccount.accountId,
    kind: BraveWallet.AccountKind.kHardware,
    uniqueKey: 'hw-eth-account',
  },
  hardware: {
    deviceId: 'trezor-1',
    path: "m/44'/60'/0'/0/0",
    vendor: BraveWallet.HardwareVendor.kTrezor,
  },
}

const hardwareSignRequest: BraveWallet.SignMessageRequest = {
  ...mockSignMessageRequest,
  id: 7,
  accountId: hardwareEthAccount.accountId,
}

const hotTransaction = deserializeTransaction({
  ...mockTransactionInfo,
  id: 'hot-tx',
  txStatus: BraveWallet.TransactionStatus.Unapproved,
  fromAccountId: mockAccount.accountId,
})

const signOperation: HardwareWalletOperation = {
  type: 'signMessage',
  id: hardwareSignRequest.id,
  accountUniqueKey: hardwareEthAccount.accountId.uniqueKey,
}

function queryDataIncludesId(
  store: ReturnType<typeof createMockStore>,
  endpointName: string,
  id: string | number,
) {
  const queries = store.getState()[walletApiReducerPath]?.queries ?? {}
  return Object.values(queries).some((query) => {
    if (
      !query
      || query.endpointName !== endpointName
      || query.status !== 'fulfilled'
      || !Array.isArray(query.data)
    ) {
      return false
    }
    return query.data.some((item) => item?.id === id)
  })
}

function renderPanel(operation: HardwareWalletOperation) {
  const store = createMockStore(
    {
      panelStateOverride: {
        selectedPanel: 'connectHardwareWallet',
        hardwareWalletOperation: operation,
      },
      uiStateOverride: {
        selectedPendingTransactionId: hotTransaction.id,
      },
    },
    {
      accountInfos: [hardwareEthAccount, mockAccount],
      selectedAccountId: hardwareEthAccount.accountId,
      signMessageRequests: [hardwareSignRequest],
      transactionInfos: [hotTransaction],
    },
  )

  const api = getAPIProxy()
  const approveTransaction = jest.fn(async () => ({
    status: true,
    errorMessage: '',
    errorUnion: {
      ...emptyProviderErrorCodeUnion,
      providerError: BraveWallet.ProviderError.kSuccess,
    },
  }))
  api.txService.approveTransaction = approveTransaction

  const view = render(
    <Provider store={store}>
      <WalletTestThemeProvider>
        <MemoryRouter>
          <ConnectHardwareWalletPanel hardwareWalletCode={undefined} />
        </MemoryRouter>
      </WalletTestThemeProvider>
    </Provider>,
  )

  return { store, approveTransaction, view }
}

async function waitForQueues(store: ReturnType<typeof createMockStore>) {
  await waitFor(() => {
    expect(
      queryDataIncludesId(store, 'getTransactions', hotTransaction.id),
    ).toBe(true)
    expect(
      queryDataIncludesId(
        store,
        'getPendingSignMessageRequests',
        hardwareSignRequest.id,
      ),
    ).toBe(true)
    expect(
      screen.getByText('BRAVE_WALLET_ADD_ACCOUNT_CONNECT'),
    ).toBeInTheDocument()
  })
}

describe('ConnectHardwareWalletPanel retry', () => {
  beforeEach(() => {
    jest.mocked(signEthMessageWithHardwareKeyring).mockResolvedValue({
      success: false,
      code: 'disconnected',
      error: 'disconnected',
    })
    jest.mocked(getDeviceNameFromDevice).mockResolvedValue({
      success: false,
      error: 'disconnected',
      code: 'disconnected',
    })
  })

  it('retries the hardware sign request and does not approve a hot transaction', async () => {
    const { store, approveTransaction, view } = renderPanel(signOperation)

    await waitForQueues(store)

    view.rerender(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <MemoryRouter>
            <ConnectHardwareWalletPanel hardwareWalletCode='deviceNotConnected' />
          </MemoryRouter>
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(signEthMessageWithHardwareKeyring).toHaveBeenCalled()
    })
    expect(approveTransaction).not.toHaveBeenCalled()
  })

  it('does not sign or approve other queues for a different hardware transaction', async () => {
    const { store, approveTransaction, view } = renderPanel({
      type: 'transaction',
      id: 'hardware-tx-not-loaded',
      accountUniqueKey: hardwareEthAccount.accountId.uniqueKey,
    })

    await waitForQueues(store)

    view.rerender(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <MemoryRouter>
            <ConnectHardwareWalletPanel hardwareWalletCode='deviceNotConnected' />
          </MemoryRouter>
        </WalletTestThemeProvider>
      </Provider>,
    )

    await act(async () => {
      await new Promise((resolve) => setTimeout(resolve, 20))
    })

    expect(signEthMessageWithHardwareKeyring).not.toHaveBeenCalled()
    expect(approveTransaction).not.toHaveBeenCalled()
  })
})
