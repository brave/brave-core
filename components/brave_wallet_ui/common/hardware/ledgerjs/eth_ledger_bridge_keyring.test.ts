/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveWallet } from '../../../constants/types'
import EthereumLedgerBridgeKeyring from './eth_ledger_bridge_keyring'
import { MockLedgerTransport } from './mock_ledger_transport'
import {
  EthLedgerDeprecatedHardwareImportScheme,
  EthLedgerLegacyHardwareImportScheme,
  EthLedgerLiveHardwareImportScheme,
  HardwareOperationResultAccounts,
  HardwareOperationError
} from '../types'
import {
  EthGetAccountResponse,
  LedgerCommand,
  UnlockResponse,
  EthSignTransactionResponse,
  EthSignPersonalMessageResponse,
  EthSignEip712MessageResponse
} from './ledger-messages'
import { Untrusted } from '../untrusted_shared_types'

const createKeyring = () => {
  let keyring = new EthereumLedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring.setTransportForTesting(transport)
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring.setBridgeForTesting(iframe)
  return { keyring, transport }
}

const mockEthereumSignatureVRS: BraveWallet.EthereumSignatureVRS = {
  vBytes: [1],
  rBytes: [2, 3],
  sBytes: [4, 5, 6]
}

const mockUntrustedSignatureVRS: Untrusted.EthereumSignatureVRS = {
  vBytes: Buffer.from([1]),
  rBytes: Buffer.from([2, 3]),
  sBytes: Buffer.from([4, 5, 6])
}

const unlockSuccessResponse: UnlockResponse = {
  id: LedgerCommand.Unlock,
  origin: window.origin,
  command: LedgerCommand.Unlock,
  payload: { success: true }
}

const unlockError: HardwareOperationError = {
  success: false,
  error: 'LedgerError',
  code: 101
}

const unlockErrorResponse: UnlockResponse = {
  id: LedgerCommand.Unlock,
  origin: window.origin,
  command: LedgerCommand.Unlock,
  payload: unlockError
}

test('getAccounts unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const result: HardwareOperationResultAccounts = await keyring.getAccounts(
    0,
    1,
    EthLedgerLiveHardwareImportScheme
  )
  expect(result).toEqual(unlockError)
})

test('getAccounts ledger live derivation path success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const getAccountsResponse1: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      publicKey: 'publicKey',
      address: 'address'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse1)
  const getAccountsResponse2: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      publicKey: 'publicKey 2',
      address: 'address 2'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse2)

  const result = await keyring.getAccounts(
    0,
    2,
    EthLedgerLiveHardwareImportScheme
  )
  expect(result).toEqual({
    success: true,
    accounts: [
      {
        address: 'address',
        derivationPath: "m/44'/60'/0'/0/0"
      },
      {
        address: 'address 2',
        derivationPath: "m/44'/60'/1'/0/0"
      }
    ]
  })
})

test('getAccounts legacy derivation path success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const getAccountsResponse1: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      publicKey: 'publicKey',
      address: 'address'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse1)
  const getAccountsResponse2: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      publicKey: 'publicKey 2',
      address: 'address 2'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse2)

  const result = await keyring.getAccounts(
    0,
    2,
    EthLedgerLegacyHardwareImportScheme
  )
  expect(result).toEqual({
    success: true,
    accounts: [
      {
        address: 'address',
        derivationPath: "m/44'/60'/0'/0"
      },
      {
        address: 'address 2',
        derivationPath: "m/44'/60'/0'/1"
      }
    ]
  })
})

test('getAccounts deprecated derivation path success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponse1: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      publicKey: 'publicKey',
      address: 'address'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse1)
  const getAccountsResponse2: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      publicKey: 'publicKey 2',
      address: 'address 2'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse2)

  const result = await keyring.getAccounts(
    0,
    2,
    EthLedgerDeprecatedHardwareImportScheme
  )
  expect(result).toEqual({
    success: true,
    accounts: [
      {
        address: 'address',
        derivationPath: "m/44'/60'/0'/0"
      },
      {
        address: 'address 2',
        derivationPath: "m/44'/60'/1'/0"
      }
    ]
  })
})

test('getAccounts ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const getAccountResponseLedgerError: EthGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: unlockError
  }

  transport.addSendCommandResponse(getAccountResponseLedgerError)
  const result: HardwareOperationResultAccounts = await keyring.getAccounts(
    0,
    1,
    EthLedgerLiveHardwareImportScheme
  )

  expect(result).toEqual(unlockError)
})

test('signTransaction unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const result = await keyring.signTransaction(
    "m/44'/60'/0'/0/0",
    'transaction'
  )
  expect(result).toEqual(unlockError)
})

test('signTransaction success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const signTransactionResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      signature: mockUntrustedSignatureVRS
    }
  }

  transport.addSendCommandResponse(signTransactionResponse)
  const result = await keyring.signTransaction("44'/501'/1'/0'", 'transaction')

  const expectedResult = {
    success: true,
    signature: mockEthereumSignatureVRS
  }
  expect(result).toEqual(expectedResult)
})

test('signTransaction ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)

  const signTransactionResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: unlockError
  }
  transport.addSendCommandResponse(signTransactionResponse)
  const result = await keyring.signTransaction(
    "m/44'/60'/0'/0/0",
    'transaction'
  )

  expect(result).toEqual(unlockError)
})

test('signPersonalMessage unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const result = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )
  const expectedResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const responsePayload: EthSignPersonalMessageResponse = {
    id: LedgerCommand.SignPersonalMessage,
    origin: window.origin,
    command: LedgerCommand.SignPersonalMessage,
    payload: {
      success: true,
      signature: { bytes: Buffer.from([1, 2, 3, 4, 5]) }
    }
  }
  transport.addSendCommandResponse(responsePayload)
  const result = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )

  expect(result).toEqual({
    success: true,
    signature: {
      bytes: [1, 2, 3, 4, 5]
    }
  })
})

test('signPersonalMessage failure after unsuccessful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)

  const signPersonalMessageResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: unlockError
  }
  transport.addSendCommandResponse(signPersonalMessageResponse)
  const result = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )
  expect(result).toEqual(unlockError)
})

test('signEip712Message unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const result = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'domainSeparatorHex',
    'hashStructMessageHex'
  )
  const expectedResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signEip712Message success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const responsePayload: EthSignEip712MessageResponse = {
    id: LedgerCommand.SignEip712Message,
    origin: window.origin,
    command: LedgerCommand.SignEip712Message,
    payload: {
      success: true,
      signature: { bytes: Buffer.from([1, 1, 2, 3, 5, 8]) }
    }
  }
  transport.addSendCommandResponse(responsePayload)
  const result = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'domainSeparatorHex',
    'hashStructMessageHex'
  )

  expect(result).toEqual({
    success: true,
    signature: {
      bytes: [1, 1, 2, 3, 5, 8]
    }
  })
})

test('signEip712Message failure after unsuccessful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const signEip712MessageResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: unlockError
  }
  transport.addSendCommandResponse(signEip712MessageResponse)
  const result = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'domainSeparatorHex',
    'hashStructMessageHex'
  )
  expect(result).toEqual(unlockError)
})
