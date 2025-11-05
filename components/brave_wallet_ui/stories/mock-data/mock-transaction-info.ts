// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import {
  BraveWallet,
  MaxPriorityFeeOptionType,
  SerializableTransactionInfo,
  StorybookCoinTypes,
  StorybookTransactionTypes,
} from '../../constants/types'
import { deserializeTransaction } from '../../utils/model-serialization-utils'
import {
  FileCoinTransactionInfo,
  ParsedTransaction,
  parseTransactionWithPrices,
} from '../../utils/tx-utils'

// Mocks
import {
  mockAccount,
  mockEthAccount,
  mockBtcAccount,
  mockFilecoinAccount,
  mockSolanaAccount,
  mockSpotPriceRegistry,
  mockZecAccount,
} from '../../common/constants/mocks'
import { mockOriginInfo, mockUniswapOriginInfo } from './mock-origin-info'
import {
  mockBasicAttentionToken,
  mockErc20TokensList,
  mockUSDCoin,
} from './mock-asset-options'
import { LiFiExchangeProxy } from '../../common/constants/registry'
import { mockEthMainnet } from './mock-networks'

export const mockTransactionInfo: SerializableTransactionInfo = {
  fromAccountId: mockAccount.accountId,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  id: '465a4d6646-kjlwf665',
  txArgs: ['0x0d8775f648430679a709e98d2b0cb6250d2887ef', '0x15ddf09c97b0000'],
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        nonce: '0x1',
        gasPrice: '100000000',
        gasLimit: '122665', // wei
        to: mockBasicAttentionToken.contractAddress,
        value: '0x15ddf09c97b0000',
        data: Array.from(new Uint8Array(24)),
        signOnly: false,
        signedTransaction: undefined,
      },
      chainId: '0x0',
      maxPriorityFeePerGas: '80410000', // (0.08041 gwei)
      maxFeePerGas: '3600000000', // (3.6 gwei)
      gasEstimation: undefined,
    },
    ethTxData: undefined,
    solanaTxData: undefined,
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined,
  },
  txHash:
    '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
  txStatus: BraveWallet.TransactionStatus.Unapproved,
  txParams: ['address', 'amount'],
  txType: BraveWallet.TransactionType.ERC20Transfer,
  createdTime: { microseconds: 0 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: mockUniswapOriginInfo,
  effectiveRecipient: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
  isRetriable: false,
  swapInfo: undefined,
}

export const mockSolanaTransactionInfo: SerializableTransactionInfo = {
  chainId: BraveWallet.SOLANA_MAINNET,
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
        numRequiredSignatures: 1,
      },
      recentBlockhash: '1',
      sendOptions: {
        preflightCommitment: '',
        skipPreflight: undefined,
        maxRetries: { maxRetries: 1 },
      },
      signTransactionParam: {
        encodedSerializedMsg: '',
        signatures: [],
      },
      feeEstimation: undefined,
      tokenAddress: '',
      staticAccountKeys: [],
      toWalletAddress: mockSolanaAccount.address,
      txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      version: 1,
    },
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined,
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
  swapInfo: undefined,
}

export const mockSOLTXInstructions: SerializableTransactionInfo = {
  chainId: BraveWallet.SOLANA_MAINNET,
  fromAccountId: mockSolanaAccount.accountId,
  id: 'sol-tx',
  txArgs: [],
  txDataUnion: {
    ethTxData1559: undefined,
    ethTxData: undefined,
    solanaTxData: {
      addressTableLookups: [],
      amount: '0',
      feePayer: '6ZpxVa8QbhoLhLUvsXUQJECZ6tRQsc7SUbeVk5HTJQEo',
      feeEstimation: {
        baseFee: BigInt(12),
        computeUnits: 1000000,
        feePerComputeUnit: BigInt(12),
      },
      instructions: [
        {
          programId: 'ComputeBudget111111111111111111111111111111',
          accountMetas: [],
          data: [2, 240, 1, 0, 0],
          decodedData: undefined,
        },
        {
          programId: 'ComputeBudget111111111111111111111111111111',
          accountMetas: [],
          data: [3, 1, 0, 0, 0, 0, 0, 0],
          decodedData: undefined,
        },
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true,
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: false,
              isWritable: true,
            },
          ],
          data: [2, 0, 0, 0, 96, 224, 200, 180, 5, 0, 0, 0],
          decodedData: {
            instructionType: 2,
            accountParams: [
              {
                localizedName: 'From account',
                name: 'from_account',
              },
              {
                localizedName: 'To account',
                name: 'to_account',
              },
            ],
            params: [
              {
                name: 'lamports',
                value: '2039280',
                type: 2,
                localizedName: 'lamports',
              },
            ],
          },
        },
      ],
      lamports: '100',
      lastValidBlockHeight: '1',
      messageHeader: {
        numReadonlySignedAccounts: 1,
        numReadonlyUnsignedAccounts: 1,
        numRequiredSignatures: 1,
      },
      recentBlockhash: '1',
      sendOptions: {
        preflightCommitment: '',
        skipPreflight: undefined,
        maxRetries: { maxRetries: 1 },
      },
      signTransactionParam: {
        encodedSerializedMsg: '',
        signatures: [],
      },
      tokenAddress: '',
      staticAccountKeys: [],
      toWalletAddress: mockSolanaAccount.address,
      txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      version: 1,
    },
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined,
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
  swapInfo: undefined,
}

export const mockATAInstruction = {
  programId: BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID,
  accountMetas: [
    {
      pubkey: mockSolanaAccount.address,
      isSigner: true,
      isWritable: true,
      addrTableLookupIndex: undefined,
    },
    {
      pubkey: 'ata',
      isSigner: false,
      isWritable: true,
      addrTableLookupIndex: undefined,
    },
    {
      pubkey: mockSolanaAccount.address,
      isSigner: true,
      isWritable: true,
      addrTableLookupIndex: undefined,
    },
    {
      pubkey: 'mint',
      isSigner: false,
      isWritable: true,
      addrTableLookupIndex: undefined,
    },
    {
      pubkey: 'systemProgram',
      isSigner: false,
      isWritable: true,
      addrTableLookupIndex: undefined,
    },
    {
      pubkey: 'tokenProgram',
      isSigner: false,
      isWritable: true,
      addrTableLookupIndex: undefined,
    },
  ],
  data: [],
  decodedData: {
    instructionType: 0,
    accountParams: [],
    params: [
      {
        name: 'lamports',
        value: '2039280',
        type: 0,
        localizedName: 'lamports',
      },
    ],
  },
}

export const mockSolanaTransactionInfoAccount: BraveWallet.AccountInfo = {
  ...mockSolanaAccount,
  accountId: mockSolanaTransactionInfo.fromAccountId,
}

export const mockSvmTxInfos: BraveWallet.TransactionInfo[] = [
  deserializeTransaction({
    ...mockSolanaTransactionInfo,
    txStatus: BraveWallet.TransactionStatus.Unapproved,
    txType: BraveWallet.TransactionType.SolanaSystemTransfer,
  }),
  deserializeTransaction({
    ...mockSolanaTransactionInfo,
    txStatus: BraveWallet.TransactionStatus.Unapproved,
    txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
  }),
]

export const mockFilSendTransaction: FileCoinTransactionInfo = {
  chainId: BraveWallet.FILECOIN_MAINNET,
  confirmedTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
  createdTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
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
      maxFee: '1000',
    },
    ethTxData: undefined,
    ethTxData1559: undefined,
    solanaTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined,
    cardanoTxData: undefined,
    polkadotTxData: undefined,
  },
  txHash: 'fil-send-tx',
  txParams: [],
  txStatus: BraveWallet.TransactionStatus.Confirmed,
  txType: BraveWallet.TransactionType.Other,
  effectiveRecipient: mockAccount.address,
  isRetriable: false,
  swapInfo: undefined,
}

export const mockedErc20ApprovalTransaction = {
  ...mockTransactionInfo,
  txType: BraveWallet.TransactionType.ERC20Approve,
  id: 'erc20-approve-tx',
}

export const mockEthSendTransaction = {
  id: 'eth-send-tx',
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
        signedTransaction: 'mockSignedTx',
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
        baseFeePerGas: '0x12',
      },
    },
  },
  txStatus: 4,
  txType: 0,
  txParams: [],
  txArgs: [],
  createdTime: {
    microseconds: 1697722990427000,
  },
  submittedTime: {
    microseconds: 1697723019222000,
  },
  confirmedTime: {
    microseconds: 1697723040798000,
  },
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: '',
  },
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  effectiveRecipient: mockEthAccount.accountId.address,
  isRetriable: false,
  swapInfo: undefined,
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
      inputs: [
        {
          address: mockBtcAccount.address,
          value: BigInt(10000000000000),
          outpointIndex: 14,
          outpointTxid: 'btc-send-tx',
        },
      ],
      outputs: [
        {
          address: 'another-btc-address',
          value: BigInt(10000000000000),
        },
        {
          address: 'another-btc-address',
          value: BigInt(10000000000000),
        },
      ],
      sendingMaxAmount: false,
      to: 'another-btc-address',
    },
  },
  txStatus: 4,
  txType: BraveWallet.TransactionType.Other,
  txParams: [],
  txArgs: [],
  createdTime: {
    microseconds: 1697722990427000,
  },
  submittedTime: {
    microseconds: 1697723019222000,
  },
  confirmedTime: {
    microseconds: 1697723040798000,
  },
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: '',
  },
  chainId: BraveWallet.BITCOIN_MAINNET,
  effectiveRecipient: mockBtcAccount.accountId.address,
  isRetriable: false,
  swapInfo: undefined,
}

export const mockZecSendTransaction: SerializableTransactionInfo = {
  id: 'Zec-send-tx-1',
  fromAccountId: mockZecAccount.accountId,
  txHash: 'Zec-send-tx',
  txDataUnion: {
    zecTxData: {
      amount: BigInt(10000000000000),
      fee: BigInt(10000000),
      inputs: [
        {
          address: mockZecAccount.address,
          value: BigInt(10000000000000),
        },
      ],
      outputs: [
        {
          address: 'another-zec-address',
          value: BigInt(10000000000000),
        },
        {
          address: 'another-zec-address',
          value: BigInt(10000000000000),
        },
      ],
      to: 'another-Zec-address',
      useShieldedPool: false,
      memo: undefined,
      sendingMaxAmount: false,
    },
  },
  txStatus: 4,
  txType: BraveWallet.TransactionType.Other,
  txParams: [],
  txArgs: [],
  createdTime: {
    microseconds: 1697722990427000,
  },
  submittedTime: {
    microseconds: 1697723019222000,
  },
  confirmedTime: {
    microseconds: 1697723040798000,
  },
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: '',
  },
  chainId: BraveWallet.Z_CASH_MAINNET,
  effectiveRecipient: mockZecAccount.accountId.address,
  isRetriable: false,
  swapInfo: undefined,
}

export const mockERC20TransferTransaction: SerializableTransactionInfo = {
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  confirmedTime: { microseconds: 0 },
  createdTime: { microseconds: 1755121283613000 },
  effectiveRecipient: mockEthAccount.accountId.address,
  fromAccountId: mockEthAccount.accountId,
  id: 'ERC20-transfer-tx-1',
  isRetriable: false,
  originInfo: mockOriginInfo,
  submittedTime: { microseconds: 0 },
  swapInfo: undefined,
  txArgs: ['0x0d8775f648430679a709e98d2b0cb6250d2887ef', '0x15ddf09c97b0000'],
  txDataUnion: {
    ethTxData1559: {
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      maxPriorityFeePerGas: '0x2faf080',
      maxFeePerGas: '0x2faf092',
      gasEstimation: {
        slowMaxPriorityFeePerGas: '0x2',
        slowMaxFeePerGas: '0x14',
        avgMaxPriorityFeePerGas: '0x2faf080',
        avgMaxFeePerGas: '0x2faf092',
        fastMaxPriorityFeePerGas: '0x59682f00',
        fastMaxFeePerGas: '0x59682f12',
        baseFeePerGas: '0x12',
      },
      baseData: {
        nonce: '0xb',
        gasPrice: '0x0',
        gasLimit: '0x5208',
        to: mockEthAccount.accountId.address,
        value: '0x5543df729c0000',
        data: [
          168, 0, 5, 168, 0, 5, 168, 0, 5, 168, 0, 5, 168, 0, 5, 168, 0, 5,
        ],
        signOnly: false,
        signedTransaction: 'mockSignedTx',
      },
    },
  },
  txHash: 'ERC20-transfer-tx-hash',
  txParams: ['address', 'uint256'],
  txStatus: BraveWallet.TransactionStatus.Unapproved,
  txType: BraveWallet.TransactionType.ERC20Transfer,
}

export const createMockERC20TransferTxArgs = ({
  amount,
  recipient,
}: {
  recipient: string
  amount: number | string
}) => {
  // (address recipient, uint256 amount)
  return [recipient, amount.toString()]
}

export const createMockERC20ApprovalTxArgs = ({
  amount,
  spender,
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
  tokenId,
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
    buyAmount,
  } = arg

  const txBase =
    coinType === BraveWallet.CoinType.FIL
      ? mockFilSendTransaction
      : deserializeTransaction(
          coinType === BraveWallet.CoinType.ETH
            ? mockTransactionInfo
            : coinType === BraveWallet.CoinType.SOL
              ? mockSolanaTransactionInfo
              : mockTransactionInfo,
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
      baseFeePerGas: '0x12',
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
      value: '1',
    },
  }

  let txArgs: string[] = []
  let swapInfo

  switch (true) {
    case isERC20Approve: {
      txArgs = createMockERC20ApprovalTxArgs({
        amount: sendApproveOrSellAmount,
        spender: fromAccount.address,
      })
    }
    case isERC20Send: {
      txArgs = createMockERC20TransferTxArgs({
        recipient: toAddress,
        amount: sendApproveOrSellAmount,
      })
      txBase.txDataUnion.ethTxData1559 = ethTxData
    }
    case isERC721Send: {
      txArgs = createMockERC721TxArgs({
        owner: fromAccount.address,
        to: toAddress,
        tokenId: tokenId || '',
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
        provider: 'lifi',
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
    swapInfo,
  }
}

const getMockTransactionType = (
  isSwapOrBridge: boolean,
  transactionType: StorybookTransactionTypes,
  coinType: BraveWallet.CoinType,
) => {
  if (isSwapOrBridge && coinType === BraveWallet.CoinType.SOL) {
    return BraveWallet.TransactionType.SolanaSwap
  }
  if (isSwapOrBridge) {
    return BraveWallet.TransactionType.ETHSwap
  }
  if (transactionType === 'Approve') {
    return BraveWallet.TransactionType.ERC20Approve
  }
  if (coinType === BraveWallet.CoinType.SOL) {
    return BraveWallet.TransactionType.SolanaSystemTransfer
  }
  return BraveWallet.TransactionType.ETHSend
}

export const getPostConfirmationStatusMockTransaction = (
  transactionType: StorybookTransactionTypes,
  transactionStatus: BraveWallet.TransactionStatus,
  transactionCoinType: StorybookCoinTypes,
) => {
  const isSwapOrBridge =
    transactionType === 'Swap' || transactionType === 'Bridge'

  const coin =
    transactionCoinType === 'BTC'
      ? BraveWallet.CoinType.BTC
      : transactionCoinType === 'FIL'
        ? BraveWallet.CoinType.FIL
        : transactionCoinType === 'SOL'
          ? BraveWallet.CoinType.SOL
          : transactionCoinType === 'ZEC'
            ? BraveWallet.CoinType.ZEC
            : BraveWallet.CoinType.ETH

  const chain =
    transactionCoinType === 'BTC'
      ? BraveWallet.BITCOIN_MAINNET
      : transactionCoinType === 'FIL'
        ? BraveWallet.FILECOIN_MAINNET
        : transactionCoinType === 'SOL'
          ? BraveWallet.SOLANA_MAINNET
          : transactionCoinType === 'ZEC'
            ? BraveWallet.Z_CASH_MAINNET
            : BraveWallet.MAINNET_CHAIN_ID

  return {
    ...mockTransactionInfo,
    chainId: chain,
    txArgs: [
      BraveWallet.TransactionType.ERC20Approve
        ? LiFiExchangeProxy
        : '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
      '0x15ddf09c97b0000',
    ],
    txDataUnion: {
      ...mockTransactionInfo.txDataUnion,
      ethTxData1559:
        coin === BraveWallet.CoinType.ETH
          ? {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559?.baseData,
                to: mockBasicAttentionToken.contractAddress,
              },
            }
          : undefined,
      solanaTxData:
        coin === BraveWallet.CoinType.SOL
          ? mockSolanaTransactionInfo.txDataUnion.solanaTxData
          : undefined,
    },
    txStatus: transactionStatus,
    txType: getMockTransactionType(isSwapOrBridge, transactionType, coin),
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
          provider: 'lifi',
        } as BraveWallet.SwapInfo)
      : undefined,
  } as SerializableTransactionInfo
}

export const mockSuggestedMaxPriorityFeeOptions: MaxPriorityFeeOptionType[] = [
  {
    id: 'slow',
    fee: '0x2171aa3351',
    duration: '28 min',
  },
  {
    id: 'average',
    fee: '0x8171aa3357',
    duration: '7 min',
  },

  {
    id: 'fast',
    fee: '0x1270aa33510',
    duration: '1 min',
  },
]

export const mockParsedERC20ApprovalTransaction: ParsedTransaction =
  parseTransactionWithPrices({
    tx: mockedErc20ApprovalTransaction,
    accounts: {
      ids: [mockEthAccount.accountId.uniqueKey],
      entities: { [mockEthAccount.accountId.uniqueKey]: mockEthAccount },
    },
    gasFee: '100',
    spotPrices: mockSpotPriceRegistry,
    tokensList: mockErc20TokensList,
    transactionAccount: mockAccount,
    transactionNetwork: mockEthMainnet,
  })

export const mockETHSwapTransaction: BraveWallet.TransactionInfo = {
  ...deserializeTransaction(mockTransactionInfo),
  txType: BraveWallet.TransactionType.ETHSwap,
  id: 'mock-eth-swap-tx',
  txStatus: BraveWallet.TransactionStatus.Unapproved,
  swapInfo: {
    fromAmount: '0xde0b6b3a7640000',
    fromAsset: '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
    fromCoin: BraveWallet.CoinType.ETH,
    fromChainId: BraveWallet.MAINNET_CHAIN_ID,
    toAmount: '0x3d6235a79608ede57a',
    toAsset: mockBasicAttentionToken.contractAddress,
    toCoin: BraveWallet.CoinType.ETH,
    toChainId: BraveWallet.MAINNET_CHAIN_ID,
    receiver: mockAccount.address,
    provider: 'lifi',
  },
}

export const mockETHNativeTokenSendTransaction = {
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  confirmedTime: { microseconds: 0 },
  createdTime: {
    microseconds: 1697722990427000,
  },
  submittedTime: {
    microseconds: 0,
  },
  effectiveRecipient: mockEthAccount.accountId.address,
  fromAccountId: mockAccount.accountId,
  id: 'eth-native-token-send-tx',
  isRetriable: false,
  originInfo: {
    originSpec: 'chrome://wallet',
    eTldPlusOne: '',
  },
  swapInfo: undefined,
  txArgs: [],
  txHash: '',
  txParams: [],
  txStatus: BraveWallet.TransactionStatus.Unapproved,
  txType: BraveWallet.TransactionType.ETHSend,
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        nonce: '',
        gasPrice: '0x5f5e100',
        gasLimit: '0x5208',
        to: mockEthAccount.accountId.address,
        value: '0xad78ebc5ac620000',
        data: [],
        signOnly: false,
        signedTransaction: undefined,
      },
      chainId: '',
      maxPriorityFeePerGas: '',
      maxFeePerGas: '',
      gasEstimation: undefined,
    },
  },
}
