/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
APIProxyControllers,
TransactionInfo,
ApproveHardwareTransactionReturnInfo,
GetTransactionInfoReturnInfo,
GetTransactionMessageToSignReturnInfo,
SignatureVRS,
kLedgerHardwareVendor,
kTrezorHardwareVendor,
ProcessHardwareSignatureReturnInfo
} from '../../constants/types'
import {
signTrezorTransaction,
signLedgerTransaction
} from '../../common/async/lib'
import { getLocale } from '../../../common/locale'
import { EthereumSignedTx } from 'trezor-connect/lib/typescript/trezor/protobuf'
import { Success, Unsuccessful } from 'trezor-connect'

const getMockedTransactionInfo = (): TransactionInfo => {
  return {
    id: '1',
    fromAddress: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
    txHash: '',
    txData: {
      baseData: {
        to: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
        value: '0x01706a99bf354000',
        data: new Uint8Array(0),
        nonce: '0x03',
        gasLimit: '0x5208',
        gasPrice: '0x22ecb25c00'
      },
      chainId: '1337',
      maxPriorityFeePerGas: '',
      maxFeePerGas: ''
    },
    txStatus: 1,
    txType: 5,
    txParams: [],
    txArgs: [],
    createdTime: { microseconds: 0 },
    submittedTime: { microseconds: 0 },
    confirmedTime: { microseconds: 0 }
  }
}

const getMockedLedgerKeyring = (expectedPath: string, expectedData: string | TransactionInfo, signed?: SignatureVRS) => {
  return {
    type: () => {
      return kLedgerHardwareVendor
    },
    signTransaction: async (path: string, data: string): Promise<SignatureVRS | undefined> => {
      expect(path).toStrictEqual(expectedPath)
      expect(data).toStrictEqual(expectedData)
      return Promise.resolve(signed)
    },
    signed: () => {
      if (!signed) {
        return
      }
      const { v, r, s } = signed
      return {
        v: '0x' + v,
        r: r,
        s: s
      }
    }
  }
}

const getMockedTrezorKeyring = (expectedPath: string, expectedData: string | TransactionInfo, signed?: Success<EthereumSignedTx> | Unsuccessful) => {
  return {
    type: () => {
      return kTrezorHardwareVendor
    },
    signTransaction: async (path: string, data: string): Promise<Success<EthereumSignedTx> | Unsuccessful | undefined> => {
      expect(path).toStrictEqual(expectedPath)
      expect(data).toStrictEqual(expectedData)
      return Promise.resolve(signed)
    },
    signed: () => {
      if (!signed) {
        return
      }
      const { v, r, s } = signed.payload as EthereumSignedTx
      return {
        v: v,
        r: r.replace('0x', ''),
        s: s.replace('0x', '')
      }
    }
  }
}

const getMockedProxyControllers = (expectedId: string,
                                   transaction?: GetTransactionInfoReturnInfo | undefined,
                                   approved?: ApproveHardwareTransactionReturnInfo,
                                   messageToSign?: GetTransactionMessageToSignReturnInfo | undefined,
                                   keyring?: any,
                                   hardwareSignature?: ProcessHardwareSignatureReturnInfo) => {
  return {
    ethJsonRpcController: {
      getChainId: async () => {
        return '0x123'
      }
    },
    ethTxController: {
      approveHardwareTransaction: (id: string): ApproveHardwareTransactionReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        return approved
      },
      getTransactionInfo: (id: string): GetTransactionInfoReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        return transaction ? { info: transaction } : undefined
      },
      getTransactionMessageToSign: (id: string): GetTransactionMessageToSignReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        return messageToSign
      },
      processHardwareSignature: (id: string, v: string, r: string, s: string): ProcessHardwareSignatureReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        if (keyring) {
          expect(v).toStrictEqual(keyring.signed().v)
          expect(r).toStrictEqual(keyring.signed().r)
          expect(s).toStrictEqual(keyring.signed().s)
        }
        return hardwareSignature
      }
    },
    getKeyringsByType (type: string) {
      expect(type).toStrictEqual(keyring.type())
      return keyring
    }
  }
}

test('Test sign Ledger transaction, approve failed', () => {
  const approved = { success: false, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved)
  const expectedError = { success: false, error: getLocale('braveWalletApproveTransactionError') }
  return expect(signLedgerTransaction(apiProxy as unknown as APIProxyControllers,
    'path', txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Ledger transaction, approved, no message to sign', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved)
  const expectedError = { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  return expect(signLedgerTransaction(apiProxy as unknown as APIProxyControllers,
    'path', txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Ledger transaction, approved, device error', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const expectedData = 'raw_message_to_sign'
  const messageToSign = { message: expectedData }
  const expectedPath = 'path'
  const mockedKeyring = getMockedLedgerKeyring(expectedPath, expectedData)
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved, messageToSign, mockedKeyring)
  const expectedError = { success: false, error: getLocale('braveWalletSignOnDeviceError') }
  return expect(signLedgerTransaction(apiProxy as unknown as APIProxyControllers,
    expectedPath, txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Ledger transaction, approved, processing error', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const expectedData = 'raw_message_to_sign'
  const messageToSign = { message: expectedData }
  const expectedPath = 'path'
  const vrs = { v: 1, r: '0xR', s: '0xS' } as SignatureVRS
  const mockedKeyring = getMockedLedgerKeyring(expectedPath, expectedData, vrs)
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved, messageToSign, mockedKeyring)
  const expectedError = { success: false, error: getLocale('braveWalletProcessTransactionError') }
  return expect(signLedgerTransaction(apiProxy as unknown as APIProxyControllers,
    expectedPath, txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Ledger transaction, approved, processed', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const expectedData = 'raw_message_to_sign'
  const messageToSign = { message: expectedData }
  const expectedPath = 'path'
  const vrs = { v: 1, r: '0xR', s: '0xS' } as SignatureVRS
  const mockedKeyring = getMockedLedgerKeyring(expectedPath, expectedData, vrs)
  const signatureResponse = { status: true }
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved, messageToSign, mockedKeyring, signatureResponse)
  return expect(signLedgerTransaction(apiProxy as unknown as APIProxyControllers,
    expectedPath, txInfo)).resolves.toStrictEqual({ success: true })
})

test('Test sign Trezor transaction, approve failed', () => {
  const approved = { success: false, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved)
  const expectedError = { success: false, error: getLocale('braveWalletApproveTransactionError') }
  return expect(signTrezorTransaction(apiProxy as unknown as APIProxyControllers,
         'path', txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Trezor transaction, approved, treansaction not found to sign', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const apiProxy = getMockedProxyControllers(txInfo.id, undefined, approved)
  const expectedError = { success: false, error: getLocale('braveWalletTransactionNotFoundSignError') }
  return expect(signTrezorTransaction(apiProxy as unknown as APIProxyControllers,
         'path', txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Trezor transaction, approved, device error', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const expectedPath = 'path'
  const signed = { success: false, payload: {  error: 'error', code: '111' } } as Unsuccessful
  const mockedKeyring = getMockedTrezorKeyring(expectedPath, txInfo, signed)
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved, undefined, mockedKeyring)
  const expectedError = { success: false, error: getLocale('braveWalletSignOnDeviceError') }
  return expect(signTrezorTransaction(apiProxy as unknown as APIProxyControllers,
    expectedPath, txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Trezor transaction, approved, processing error', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const expectedPath = 'path'
  const vrs = { v: '0xV', r: '0xR', s: '0xS' } as EthereumSignedTx
  const signed = { success: true, payload: vrs } as Success<EthereumSignedTx>
  const mockedKeyring = getMockedTrezorKeyring(expectedPath, txInfo, signed)
  const signatureResponse = { status: false }
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved, undefined, mockedKeyring, signatureResponse)
  const expectedError = { success: false, error: getLocale('braveWalletProcessTransactionError') }
  return expect(signTrezorTransaction(apiProxy as unknown as APIProxyControllers,
    expectedPath, txInfo)).resolves.toStrictEqual(expectedError)
})

test('Test sign Trezor transaction, approved, processed', () => {
  const approved = { success: true, message: 'test' }
  const txInfo = getMockedTransactionInfo()
  const expectedPath = 'path'
  const vrs = { v: '0xV', r: '0xR', s: '0xS' } as EthereumSignedTx
  const signed = { success: true, payload: vrs } as Success<EthereumSignedTx>
  const mockedKeyring = getMockedTrezorKeyring(expectedPath, txInfo, signed)
  const signatureResponse = { status: true }
  const apiProxy = getMockedProxyControllers(txInfo.id, txInfo, approved, undefined, mockedKeyring, signatureResponse)
  return expect(signTrezorTransaction(apiProxy as unknown as APIProxyControllers,
    expectedPath, txInfo)).resolves.toStrictEqual({ success: true })
})
