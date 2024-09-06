// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
  StorybookTransactionTypes
} from '../../constants/types'
import { deserializeTransaction } from '../../utils/model-serialization-utils'
import { FileCoinTransactionInfo } from '../../utils/tx-utils'

// Mocks
import {
  mockAccount,
  mockBtcAccount,
  mockFilecoinAccount,
  mockSolanaAccount,
  mockSolanaAccountInfo,
  mockZecAccount
} from '../../common/constants/mocks'
import { mockOriginInfo } from './mock-origin-info'
import { mockEthAccount } from './mock-wallet-accounts'
import { mockBasicAttentionToken, mockUSDCoin } from './mock-asset-options'
import { LiFiExchangeProxy } from '../../common/constants/registry'

export const mockTransactionInfo: SerializableTransactionInfo = {
  fromAccountId: mockAccount.accountId,
  fromAddress: mockAccount.address,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  id: '465a4d6646-kjlwf665',
  txArgs: ['0x0d8775f648430679a709e98d2b0cb6250d2887ef', '0x15ddf09c97b0000'],
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        nonce: '0x1',
        gasPrice: '150',
        gasLimit: '21000',
        to: '2',
        value: '0x15ddf09c97b0000',
        data: Array.from(new Uint8Array(24)),
        signOnly: false,
        signedTransaction: undefined
      },
      chainId: '0x0',
      maxPriorityFeePerGas: '1',
      maxFeePerGas: '1',
      gasEstimation: undefined
    },
    ethTxData: undefined,
    solanaTxData: undefined,
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined
  },
  txHash:
    '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
  txStatus: BraveWallet.TransactionStatus.Unapproved,
  txParams: ['address', 'amount'],
  txType: BraveWallet.TransactionType.ERC20Transfer,
  createdTime: { microseconds: 0 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: mockOriginInfo,
  effectiveRecipient: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
  isRetriable: false,
  swapInfo: undefined
}

export const mockSolanaTransactionInfo: SerializableTransactionInfo = {
  chainId: BraveWallet.SOLANA_MAINNET,
  fromAddress: mockSolanaAccount.address,
  fromAccountId: mockSolanaAccount.accountId,
  id: 'sol-tx',
  txArgs: [],
  txDataUnion: {
    ethTxData1559: undefined,
    ethTxData: undefined,
    solanaTxData: {
      addressTableLookups: [],
      amount: '',
      feePayer: '',
      instructions: [],
      lamports: '100',
      lastValidBlockHeight: '1',
      messageHeader: {
        numReadonlySignedAccounts: 1,
        numReadonlyUnsignedAccounts: 1,
        numRequiredSignatures: 1
      },
      recentBlockhash: '1',
      sendOptions: {
        preflightCommitment: '',
        skipPreflight: undefined,
        maxRetries: { maxRetries: 1 }
      },
      signTransactionParam: {
        encodedSerializedMsg: '',
        signatures: []
      },
      feeEstimation: undefined,
      tokenAddress: '',
      staticAccountKeys: [],
      toWalletAddress: mockSolanaAccountInfo.address,
      txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      version: 1
    },
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined
  },
  txHash:
    '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
  txStatus: 0,
  txParams: ['address', 'amount'],
  txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
  createdTime: { microseconds: 0 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: mockOriginInfo,
  effectiveRecipient: undefined,
  isRetriable: false,
  swapInfo: undefined
}

export const mockSolanaTransactionInfoAccount: BraveWallet.AccountInfo = {
  ...mockSolanaAccount,
  address: mockSolanaTransactionInfo.fromAddress || '',
  accountId: mockSolanaTransactionInfo.fromAccountId
}

export const mockSvmTxInfos: BraveWallet.TransactionInfo[] = [
  deserializeTransaction({
    ...mockSolanaTransactionInfo,
    fromAddress: mockSolanaTransactionInfoAccount.address,
    txStatus: BraveWallet.TransactionStatus.Unapproved,
    txType: BraveWallet.TransactionType.SolanaSystemTransfer
  }),
  deserializeTransaction({
    ...mockSolanaTransactionInfo,
    fromAddress: mockSolanaTransactionInfoAccount.address,
    txStatus: BraveWallet.TransactionStatus.Unapproved,
    txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer
  })
]

export const mockFilSendTransaction: FileCoinTransactionInfo = {
  chainId: BraveWallet.FILECOIN_MAINNET,
  confirmedTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
  createdTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
  fromAddress: mockFilecoinAccount.address,
  fromAccountId: mockFilecoinAccount.accountId,
  id: 'fil-send-tx',
  originInfo: undefined,
  submittedTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
  txArgs: [],
  txDataUnion: {
    filTxData: {
      to: mockFilecoinAccount.address,
      value: '1000',
      nonce: '1',
      gasFeeCap: '100',
      gasLimit: '200',
      gasPremium: '1',
      maxFee: '1000'
    },
    ethTxData: undefined,
    ethTxData1559: undefined,
    solanaTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined
  },
  txHash: 'fil-send-tx',
  txParams: [],
  txStatus: BraveWallet.TransactionStatus.Confirmed,
  txType: BraveWallet.TransactionType.Other,
  effectiveRecipient: mockAccount.address,
  isRetriable: false,
  swapInfo: undefined
}

export const mockedErc20ApprovalTransaction = {
  ...mockTransactionInfo,
  txType: BraveWallet.TransactionType.ERC20Approve
}

export const mockEthSendTransaction = {
  id: '8e41ec0c-9e62-45fc-904f-a25b42275a1a',
  fromAddress: mockAccount.address,
  fromAccountId: mockAccount.accountId,
  txHash: '0xbabaaaaaaaaaaa',
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        nonce: '0xb',
        gasPrice: '0x0',
        gasLimit: '0x5208',
        to: mockEthAccount.accountId.address,
        value: '0x5543df729c0000',
        data: [],
        signOnly: false,
        signedTransaction: 'mockSignedTx'
      },
      chainId: '0xaa36a7',
      maxPriorityFeePerGas: '0x2faf080',
      maxFeePerGas: '0x2faf092',
      gasEstimation: {
        slowMaxPriorityFeePerGas: '0x2',
        slowMaxFeePerGas: '0x14',
        avgMaxPriorityFeePerGas: '0x2faf080',
        avgMaxFeePerGas: '0x2faf092',
        fastMaxPriorityFeePerGas: '0x59682f00',
        fastMaxFeePerGas: '0x59682f12',
        baseFeePerGas: '0x12'
      }
    }
  },
  txStatus: 4,
  txType: 0,
  txParams: [],
  txArgs: [],
  createdTime: {
    microseconds: 1697722990427000
  },
  submittedTime: {
    microseconds: 1697723019222000
  },
  confirmedTime: {
    microseconds: 1697723040798000
  },
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: ''
  },
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  effectiveRecipient: mockEthAccount.accountId.address,
  isRetriable: false,
  swapInfo: undefined
}

export const mockBtcSendTransaction = {
  id: 'btc-send-tx-1',
  fromAddress: mockBtcAccount.address,
  fromAccountId: mockBtcAccount.accountId,
  txHash: 'btc-send-tx',
  txDataUnion: {
    btcTxData: {
      amount: BigInt(10000000000000),
      fee: BigInt(10000000),
      inputs: [],
      outputs: [],
      sendingMaxAmount: false,
      to: 'another-btc-address'
    }
  },
  txStatus: 4,
  txType: BraveWallet.TransactionType.Other,
  txParams: [],
  txArgs: [],
  createdTime: {
    microseconds: 1697722990427000
  },
  submittedTime: {
    microseconds: 1697723019222000
  },
  confirmedTime: {
    microseconds: 1697723040798000
  },
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: ''
  },
  chainId: BraveWallet.BITCOIN_MAINNET,
  effectiveRecipient: mockBtcAccount.accountId.address,
  isRetriable: false,
  swapInfo: undefined
}

export const mockZecSendTransaction = {
  id: 'Zec-send-tx-1',
  fromAddress: mockZecAccount.address,
  fromAccountId: mockZecAccount.accountId,
  txHash: 'Zec-send-tx',
  txDataUnion: {
    zecTxData: {
      amount: BigInt(10000000000000),
      fee: BigInt(10000000),
      inputs: [],
      outputs: [],
      to: 'another-Zec-address'
    }
  },
  txStatus: 4,
  txType: BraveWallet.TransactionType.Other,
  txParams: [],
  txArgs: [],
  createdTime: {
    microseconds: 1697722990427000
  },
  submittedTime: {
    microseconds: 1697723019222000
  },
  confirmedTime: {
    microseconds: 1697723040798000
  },
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: ''
  },
  chainId: BraveWallet.Z_CASH_MAINNET,
  effectiveRecipient: mockZecAccount.accountId.address,
  isRetriable: false,
  swapInfo: undefined
}

export const createMockERC20TransferTxArgs = ({
  amount,
  recipient
}: {
  recipient: string
  amount: number | string
}) => {
  // (address recipient, uint256 amount)
  return [recipient, amount.toString()]
}

export const createMockERC20ApprovalTxArgs = ({
  amount,
  spender
}: {
  spender: string
  amount: number | string
}) => {
  // (address spender, uint256 amount)
  return [spender, amount.toString()]
}

export const createMockERC721TxArgs = ({
  owner,
  to,
  tokenId
}: {
  owner: string
  to: string
  tokenId: number | string
}) => {
  // (address owner, address to, uint256 tokenId)
  return [owner, to, tokenId.toString()]
}

export const createMockTransactionInfo = (arg: {
  toAddress: string
  fromAccount: BraveWallet.AccountInfo
  sendApproveOrSellAssetContractAddress: string
  sendApproveOrSellAmount: string
  buyAmount?: string
  buyAssetContractAddress?: string
  isERC20Send?: boolean
  isERC20Approve?: boolean
  isERC721Send?: boolean
  isSwap?: boolean
  coinType: BraveWallet.CoinType
  chainId: string
  tokenId?: string
}): BraveWallet.TransactionInfo => {
  const {
    chainId,
    coinType,
    fromAccount,
    isERC20Send,
    isSwap,
    sendApproveOrSellAmount,
    sendApproveOrSellAssetContractAddress,
    toAddress,
    buyAssetContractAddress,
    isERC20Approve,
    isERC721Send,
    tokenId,
    buyAmount
  } = arg

  const txBase =
    coinType === BraveWallet.CoinType.FIL
      ? mockFilSendTransaction
      : deserializeTransaction(
          coinType === BraveWallet.CoinType.ETH
            ? mockTransactionInfo
            : coinType === BraveWallet.CoinType.SOL
            ? mockSolanaTransactionInfo
            : mockTransactionInfo
        )

  const ethTxData = {
    ...txBase.txDataUnion.ethTxData1559,
    chainId,
    maxPriorityFeePerGas: '0x2faf080',
    maxFeePerGas: '0x2faf092',
    gasEstimation: {
      slowMaxPriorityFeePerGas: '0x2',
      slowMaxFeePerGas: '0x14',
      avgMaxPriorityFeePerGas: '0x2faf080',
      avgMaxFeePerGas: '0x2faf092',
      fastMaxPriorityFeePerGas: '0x59682f00',
      fastMaxFeePerGas: '0x59682f12',
      baseFeePerGas: '0x12'
    },
    baseData: {
      ...txBase.txDataUnion.ethTxData1559?.baseData,
      data: [],
      gasLimit: '1',
      gasPrice: '1',
      nonce: '1',
      signedTransaction: 'signed',
      signOnly: false,
      to: sendApproveOrSellAssetContractAddress,
      value: '1'
    }
  }

  let txArgs: string[] = []
  let swapInfo

  switch (true) {
    case isERC20Approve: {
      txArgs = createMockERC20ApprovalTxArgs({
        amount: sendApproveOrSellAmount,
        spender: fromAccount.address
      })
    }
    case isERC20Send: {
      txArgs = createMockERC20TransferTxArgs({
        recipient: toAddress,
        amount: sendApproveOrSellAmount
      })
      txBase.txDataUnion.ethTxData1559 = ethTxData
    }
    case isERC721Send: {
      txArgs = createMockERC721TxArgs({
        owner: fromAccount.address,
        to: toAddress,
        tokenId: tokenId || ''
      })
      txBase.txDataUnion.ethTxData1559 = ethTxData
    }
    case isSwap: {
      swapInfo = {
        fromCoin: BraveWallet.CoinType.ETH,
        fromChainId: chainId,
        fromAsset: sendApproveOrSellAssetContractAddress,
        fromAmount: sendApproveOrSellAmount,
        toCoin: BraveWallet.CoinType.ETH,
        toChainId: chainId,
        toAsset: buyAssetContractAddress,
        toAmount: buyAmount || '',
        receiver: toAddress,
        provider: 'lifi'
      } as BraveWallet.SwapInfo
    }
  }

  if (coinType === BraveWallet.CoinType.SOL) {
    txBase.txDataUnion.solanaTxData = txBase.txDataUnion.solanaTxData!
    txBase.txDataUnion.solanaTxData.tokenAddress =
      sendApproveOrSellAssetContractAddress
  }

  return {
    ...txBase,
    id: `${txBase.id}--${JSON.stringify(arg)}`,
    chainId,
    fromAccountId: fromAccount.accountId,
    txArgs,
    swapInfo
  }
}

const getMockTransactionType = (
  isSwapOrBridge: boolean,
  transactionType: StorybookTransactionTypes
) => {
  if (isSwapOrBridge) {
    return BraveWallet.TransactionType.ETHSwap
  }
  if (transactionType === 'Approve') {
    return BraveWallet.TransactionType.ERC20Approve
  }
  return BraveWallet.TransactionType.ETHSend
}

export const getPostConfirmationStatusMockTransaction = (
  transactionType: StorybookTransactionTypes,
  transactionStatus: BraveWallet.TransactionStatus
) => {
  const isSwapOrBridge =
    transactionType === 'Swap' || transactionType === 'Bridge'

  return {
    ...mockTransactionInfo,
    txArgs: [
      BraveWallet.TransactionType.ERC20Approve
        ? LiFiExchangeProxy
        : '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
      '0x15ddf09c97b0000'
    ],
    txDataUnion: {
      ...mockTransactionInfo.txDataUnion,
      ethTxData1559: {
        ...mockTransactionInfo.txDataUnion.ethTxData1559,
        baseData: {
          ...mockTransactionInfo.txDataUnion.ethTxData1559?.baseData,
          to: mockBasicAttentionToken.contractAddress
        }
      }
    },
    txStatus: transactionStatus,
    txType: getMockTransactionType(isSwapOrBridge, transactionType),
    swapInfo: isSwapOrBridge
      ? ({
          fromCoin: BraveWallet.CoinType.ETH,
          fromChainId: BraveWallet.MAINNET_CHAIN_ID,
          fromAsset: mockBasicAttentionToken.contractAddress,
          fromAmount: '9996544123665456564888',
          toCoin: BraveWallet.CoinType.ETH,
          toChainId:
            transactionType === 'Bridge'
              ? BraveWallet.SEPOLIA_CHAIN_ID
              : BraveWallet.MAINNET_CHAIN_ID,
          toAsset: mockUSDCoin.contractAddress,
          toAmount: '111111111111111',
          receiver: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
          provider: 'lifi'
        } as BraveWallet.SwapInfo)
      : undefined
  } as SerializableTransactionInfo
}
