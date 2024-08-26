/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  EthereumSignedTx,
  Success,
  Unsuccessful
} from '../hardware/trezor/trezor-connect-types'
import {
  BraveWallet,
  GetNonceForHardwareTransactionReturnInfo,
  ProcessHardwareSignatureReturnInfo
} from '../../constants/types'
import {
  signTrezorTransaction,
  signLedgerEthereumTransaction,
  signLedgerFilecoinTransaction,
  signLedgerSolanaTransaction
} from './hardware'
import WalletApiProxy from '../../common/wallet_api_proxy'
import { getLocale } from '../../../common/locale'
import { getMockedTransactionInfo } from '../constants/mocks'
import {
  SignatureVRS,
  SignHardwareOperationResult,
  SignHardwareTransactionType
} from '../hardware/types'
import LedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import { FilSignedLotusMessage } from '../hardware/ledgerjs/ledger-messages'

const getMockedLedgerEthKeyring = (
  expectedPath: string,
  expectedData: string | BraveWallet.TransactionInfo,
  signed?: SignHardwareOperationResult
) => {
  return {
    type: (): BraveWallet.HardwareVendor => {
      return BraveWallet.HardwareVendor.kLedger
    },
    signTransaction: async (
      path: string,
      data: string
    ): Promise<SignHardwareOperationResult> => {
      expect(path).toStrictEqual(expectedPath)
      expect(data).toStrictEqual(expectedData)
      if (signed) {
        return Promise.resolve(signed)
      }
      return Promise.resolve({ success: false })
    },
    signed: () => {
      if (!signed) {
        return
      }
      const { v, r, s } = signed.payload as EthereumSignedTx
      return {
        v: '0x' + v,
        r: r,
        s: s
      }
    }
  }
}

const getMockedLedgerFilKeyring = (
  expectedMessage: string | BraveWallet.TransactionInfo,
  signed?: SignHardwareOperationResult
) => {
  return {
    type: (): BraveWallet.HardwareVendor => {
      return BraveWallet.HardwareVendor.kLedger
    },
    signTransaction: async (
      message: string
    ): Promise<SignHardwareOperationResult> => {
      expect(message).toStrictEqual(expectedMessage)
      if (signed) {
        return Promise.resolve(signed)
      }
      return Promise.resolve({ success: false })
    },
    signed: () => {
      if (!signed) {
        return
      }
      return {
        Message: expectedMessage,
        Signature: {
          Type: 1,
          Data: 'signature'
        }
      }
    }
  }
}

const getMockedLedgerSolKeyring = (
  expectedMessage: Buffer,
  signed?: SignHardwareOperationResult
) => {
  return {
    type: (): BraveWallet.HardwareVendor => {
      return BraveWallet.HardwareVendor.kLedger
    },
    signTransaction: async (
      path: string,
      message: Buffer
    ): Promise<SignHardwareOperationResult> => {
      expect(message).toStrictEqual(expectedMessage)
      if (signed) {
        return Promise.resolve(signed)
      }
      return Promise.resolve({ success: false })
    }
  }
}

const getMockedTrezorKeyring = (
  expectedDevicePath: string,
  expectedData: string | BraveWallet.TransactionInfo,
  signed?: Success<EthereumSignedTx> | Unsuccessful
) => {
  return {
    signTransaction: async (
      path: string,
      data: string
    ): Promise<Success<EthereumSignedTx> | Unsuccessful | undefined> => {
      expect(path).toStrictEqual(expectedDevicePath)
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
        r: r,
        s: s
      }
    }
  }
}

const getMockedProxyServices = (
  expectedChainId: string,
  expectedId: string,
  nonce?: GetNonceForHardwareTransactionReturnInfo,
  messageToSign?: BraveWallet.MessageToSignUnion | undefined,
  hardwareSignature?: ProcessHardwareSignatureReturnInfo,
  filSignedTransaction?: string | undefined,
  solSignature?: Buffer | undefined
) => {
  return {
    jsonRpcService: {
      getChainId: async () => {
        return '0x123'
      }
    },
    txService: {
      getTransactionMessageToSign: (
        coinType: BraveWallet.CoinType,
        chainId: string,
        id: string
      ) => {
        expect(id).toStrictEqual(expectedId)
        expect(chainId).toStrictEqual(expectedChainId)
        return { message: messageToSign }
      }
    },
    ethTxManagerProxy: {
      getNonceForHardwareTransaction: (
        chainId: string,
        id: string
      ): GetNonceForHardwareTransactionReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        expect(chainId).toStrictEqual(expectedChainId)
        return nonce
      },
      processHardwareSignature: (
        chainId: string,
        id: string,
        v: string,
        r: string,
        s: string
      ): ProcessHardwareSignatureReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        expect(chainId).toStrictEqual(expectedChainId)
        expect(v.startsWith('0x')).toStrictEqual(true)
        expect(r.startsWith('0x')).toStrictEqual(true)
        expect(s.startsWith('0x')).toStrictEqual(true)
        return hardwareSignature
      }
    },
    filTxManagerProxy: {
      processFilHardwareSignature: (
        chainId: string,
        id: string,
        signedTx: string
      ): ProcessHardwareSignatureReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        expect(chainId).toStrictEqual(expectedChainId)
        expect(signedTx).toStrictEqual(filSignedTransaction)
        return hardwareSignature
      }
    },
    solanaTxManagerProxy: {
      processSolanaHardwareSignature: (
        chainId: string,
        id: string,
        signature: number[]
      ): ProcessHardwareSignatureReturnInfo | undefined => {
        expect(id).toStrictEqual(expectedId)
        expect(chainId).toStrictEqual(expectedChainId)
        expect([...solSignature!]).toStrictEqual(signature)
        return hardwareSignature
      }
    }
  }
}

const signEthTransactionWithLedger = (
  vrs?: SignatureVRS,
  signatureResponse?: boolean
): Promise<SignHardwareOperationResult> => {
  const txInfo = getMockedTransactionInfo()
  const expectedData = 'raw_message_to_sign'
  const messageToSign: BraveWallet.MessageToSignUnion = {
    messageStr: expectedData,
    messageBytes: undefined
  }
  const expectedPath = 'path'
  const signTransactionResult = vrs
    ? { success: true, payload: vrs }
    : { success: false }
  const mockedKeyring = getMockedLedgerEthKeyring(
    expectedPath,
    expectedData,
    signTransactionResult as SignHardwareOperationResult
  )
  const signed = signatureResponse ? { status: signatureResponse } : undefined
  const apiProxy = getMockedProxyServices(
    txInfo.chainId,
    txInfo.id,
    { nonce: '0x1' },
    messageToSign,
    signed
  )
  return signLedgerEthereumTransaction(
    apiProxy as unknown as WalletApiProxy,
    expectedPath,
    txInfo,
    BraveWallet.CoinType.ETH,
    mockedKeyring as unknown as LedgerBridgeKeyring
  )
}

const signSolTransactionWithLedger = (
  expectedSignature: Buffer,
  signatureResponse?: boolean
): Promise<SignHardwareOperationResult> => {
  const txInfo = getMockedTransactionInfo()
  const expectedData = Buffer.from('raw_message_to_sign')
  const expectedPath = 'path'
  const messageToSign: BraveWallet.MessageToSignUnion = {
    messageStr: undefined,
    messageBytes: [...expectedData]
  }
  const signTransactionResult = expectedSignature
    ? { success: true, payload: expectedSignature }
    : { success: false }
  const mockedKeyring = getMockedLedgerSolKeyring(
    expectedData,
    signTransactionResult
  )
  const signed = signatureResponse ? { status: signatureResponse } : undefined
  const apiProxy = getMockedProxyServices(
    txInfo.chainId,
    txInfo.id,
    { nonce: '1' },
    messageToSign,
    signed,
    undefined,
    Buffer.from('signature')
  )
  return signLedgerSolanaTransaction(
    apiProxy as unknown as WalletApiProxy,
    expectedPath,
    txInfo,
    BraveWallet.CoinType.SOL,
    mockedKeyring as unknown as SolanaLedgerBridgeKeyring
  )
}

const signFilTransactionWithLedger = (
  expectedSignature: FilSignedLotusMessage,
  signatureResponse?: boolean
): Promise<SignHardwareOperationResult> => {
  const txInfo = getMockedTransactionInfo()
  const expectedData = 'raw_message_to_sign'
  const messageToSign: BraveWallet.MessageToSignUnion = {
    messageStr: expectedData,
    messageBytes: undefined
  }
  const signTransactionResult = expectedSignature
    ? { success: true, payload: expectedSignature }
    : { success: false }
  const mockedKeyring = getMockedLedgerFilKeyring(
    expectedData,
    signTransactionResult as SignHardwareOperationResult
  )
  const signed = signatureResponse ? { status: signatureResponse } : undefined
  const apiProxy = getMockedProxyServices(
    txInfo.chainId,
    txInfo.id,
    { nonce: '1' },
    messageToSign,
    signed,
    JSON.stringify(expectedSignature)
  )
  return signLedgerFilecoinTransaction(
    apiProxy as unknown as WalletApiProxy,
    txInfo,
    BraveWallet.CoinType.FIL,
    mockedKeyring as unknown as FilecoinLedgerBridgeKeyring
  )
}

const hardwareTransactionErrorResponse = (
  errorId: string,
  code: string = ''
): SignHardwareTransactionType => {
  return { success: false, error: getLocale(errorId) }
}
const hardwareTransactionErrorResponseWithCode = (
  errorId: string,
  code: string = ''
): SignHardwareOperationResult => {
  return { success: false, error: getLocale(errorId), code: code }
}
const signTransactionWithTrezor = (
  signed: Success<EthereumSignedTx> | Unsuccessful,
  signatureResponse?: ProcessHardwareSignatureReturnInfo
) => {
  const txInfo = getMockedTransactionInfo()
  const expectedPath = 'path'
  const mockedKeyring = getMockedTrezorKeyring(expectedPath, txInfo, signed)
  const apiProxy = getMockedProxyServices(
    txInfo.chainId,
    txInfo.id,
    { nonce: '0x03' },
    undefined,
    signatureResponse
  )
  return signTrezorTransaction(
    apiProxy as unknown as WalletApiProxy,
    expectedPath,
    txInfo,
    mockedKeyring as unknown as TrezorBridgeKeyring
  )
}

test('Test sign Ledger transaction, approved, no message to sign', () => {
  const txInfo = getMockedTransactionInfo()
  const apiProxy = getMockedProxyServices(txInfo.chainId, txInfo.id, {
    nonce: '0x1'
  })
  return expect(
    signLedgerEthereumTransaction(
      apiProxy as unknown as WalletApiProxy,
      'path',
      txInfo,
      BraveWallet.CoinType.ETH
    )
  ).resolves.toStrictEqual(
    hardwareTransactionErrorResponse('braveWalletNoMessageToSignError')
  )
})

test('Test sign Ledger transaction, approved, device error', () => {
  return expect(signEthTransactionWithLedger()).resolves.toStrictEqual(
    hardwareTransactionErrorResponseWithCode('braveWalletSignOnDeviceError')
  )
})

test('Test sign Ledger transaction, approved, processing error', () => {
  return expect(
    signEthTransactionWithLedger({ v: 1, r: 'R', s: 'S' })
  ).resolves.toStrictEqual(
    hardwareTransactionErrorResponse('braveWalletProcessTransactionError')
  )
})

test('Test sign Ledger transaction, approved, processed', () => {
  return expect(
    signEthTransactionWithLedger({ v: 1, r: 'R', s: 'S' }, true)
  ).resolves.toStrictEqual({ success: true })
})

test('Test sign Trezor transaction, approve failed', () => {
  const txInfo = getMockedTransactionInfo()
  const apiProxy = getMockedProxyServices(txInfo.chainId, txInfo.id, {
    nonce: ''
  })
  return expect(
    signTrezorTransaction(apiProxy as unknown as WalletApiProxy, 'path', txInfo)
  ).resolves.toStrictEqual(
    hardwareTransactionErrorResponse('braveWalletApproveTransactionError')
  )
})

test('Test sign Trezor transaction, approved, device error', () => {
  return expect(
    signTransactionWithTrezor({
      success: false,
      payload: { error: 'error', code: '111' }
    })
  ).resolves.toStrictEqual(
    hardwareTransactionErrorResponse('braveWalletSignOnDeviceError')
  )
})

test('Test sign Trezor transaction, approved, processing error', () => {
  return expect(
    signTransactionWithTrezor(
      { id: 1, success: true, payload: { v: '0xV', r: '0xR', s: '0xS' } },
      { status: false }
    )
  ).resolves.toStrictEqual(
    hardwareTransactionErrorResponse('braveWalletProcessTransactionError')
  )
})

test('Test sign Trezor transaction, approved, processed', () => {
  return expect(
    signTransactionWithTrezor(
      { id: 1, success: true, payload: { v: '0xV', r: '0xR', s: '0xS' } },
      { status: true }
    )
  ).resolves.toStrictEqual({ success: true })
})

test('Test sign Ledger FIL transaction, signed', () => {
  const message = {
    'From': 't1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q',
    'GasFeeCap': '3',
    'GasLimit': 4,
    'GasPremium': '2',
    'Method': 0,
    'Nonce': 1,
    'Params': '',
    'To': 't1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
    'Value': '11'
  }
  const expectedSignature = {
    Message: message,
    Signature: {
      Type: 1,
      Data: 'signed'
    }
  }

  return expect(
    signFilTransactionWithLedger(expectedSignature, true)
  ).resolves.toStrictEqual({ success: true })
})

test('Test sign Ledger SOL transaction, signed', () => {
  const expectedSignature = Buffer.from('signature')
  return expect(
    signSolTransactionWithLedger(expectedSignature, true)
  ).resolves.toStrictEqual({ success: true })
})
