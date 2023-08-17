// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import {
  mockAccount,
  mockFilecoinAccount,
  mockSolanaAccount,
  mockSolanaAccountInfo
} from '../../common/constants/mocks'
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../constants/types'
import { deserializeTransaction } from '../../utils/model-serialization-utils'
import { FileCoinTransactionInfo } from '../../utils/tx-utils'

// Mocks
import { mockOriginInfo } from './mock-origin-info'

export const mockTransactionInfo: SerializableTransactionInfo = {
  fromAccountId: mockAccount.accountId,
  chainId: BraveWallet.GOERLI_CHAIN_ID,
  fromAddress: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
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
      maxPriorityFeePerGas: '',
      maxFeePerGas: '',
      gasEstimation: undefined
    },
    ethTxData: undefined,
    solanaTxData: undefined,
    filTxData: undefined
  },
  txHash: '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
  txStatus: 0,
  txParams: ['address', 'amount'],
  txType: BraveWallet.TransactionType.ERC20Transfer,
  createdTime: { microseconds: 0 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: mockOriginInfo,
  groupId: undefined,
  effectiveRecipient: '0x0d8775f648430679a709e98d2b0cb6250d2887ef'
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
      messageHeader: {numReadonlySignedAccounts: 1, numReadonlyUnsignedAccounts: 1, numRequiredSignatures: 1},
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
      splTokenMintAddress: '',
      staticAccountKeys: [],
      toWalletAddress: mockSolanaAccountInfo.address,
      txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      version: 1,
    },
    filTxData: undefined
  },
  txHash: '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
  txStatus: 0,
  txParams: ['address', 'amount'],
  txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
  createdTime: { microseconds: 0 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: mockOriginInfo,
  groupId: undefined,
  effectiveRecipient: undefined
}

export const mockFilSendTransaction: FileCoinTransactionInfo = {
  chainId: BraveWallet.FILECOIN_MAINNET,
  confirmedTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
  createdTime: { microseconds: BigInt(new Date().getUTCMilliseconds()) },
  fromAddress: mockFilecoinAccount.address,
  fromAccountId: mockFilecoinAccount.accountId,
  groupId: undefined,
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
    solanaTxData: undefined
  },
  txHash: 'fil-send-tx',
  txParams: [],
  txStatus: BraveWallet.TransactionStatus.Confirmed,
  txType: BraveWallet.TransactionType.Other,
  effectiveRecipient: mockAccount.address
}

export const mockedErc20ApprovalTransaction = {
  ...mockTransactionInfo,
  txType: BraveWallet.TransactionType.ERC20Approve
}

export const createMockERC20TransferTxArgs = ({
  amount,
  recipient
}:{
  recipient: string,
  amount: number | string
}) => {
  // (address recipient, uint256 amount)
  return [recipient, amount.toString()]
}

export const createMockERC20ApprovalTxArgs = ({
  amount,
  spender
}:{
  spender: string,
  amount: number | string
}) => {
  // (address spender, uint256 amount)
  return [spender, amount.toString()]
}

export const createMockERC721TxArgs = ({
  owner,
  to,
  tokenId
}:{
  owner: string,
  to: string,
  tokenId: number | string
}) => {
  // (address owner, address to, uint256 tokenId)
  return [owner, to, tokenId.toString()]
}

export const createMockEthSwapTxArgs = ({
  buyTokenContractAddress,
  sellTokenContractAddress,
  buyAmountWei,
  sellAmountWei
}: {
  sellTokenContractAddress: string
  buyTokenContractAddress: string,
  sellAmountWei: string,
  buyAmountWei: string,
}) => {
  const fillPath = `${sellTokenContractAddress}${
    //
    buyTokenContractAddress.replace('0x', '')
  }`
  // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
  return [fillPath, sellAmountWei, buyAmountWei]
}

export const createMockTransactionInfo = (arg: {
  toAddress: string
  fromAddress: string,
  sendApproveOrSellAssetContractAddress: string
  sendApproveOrSellAmount: string
  buyAmount?: string
  buyAssetContractAddress?: string
  isERC20Send?: boolean
  isERC20Approve?: boolean
  isERC721Send?: boolean
  isSwap?: boolean,
  coinType: BraveWallet.CoinType
  chainId: string
  tokenId?: string
}): BraveWallet.TransactionInfo => {
  const {
    chainId,
    coinType,
    fromAddress,
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

  let txArgs: string[] = []

  switch (true) {
    case isERC20Approve: {
      txArgs = createMockERC20ApprovalTxArgs({
        amount: sendApproveOrSellAmount,
        spender: fromAddress
      })
    }
    case isERC20Send: {
      txArgs = createMockERC20TransferTxArgs({
        recipient: toAddress,
        amount: sendApproveOrSellAmount
      })
    }
    case isERC721Send: {
      txArgs = createMockERC721TxArgs({
        owner: fromAddress,
        to: toAddress,
        tokenId: tokenId || ''
      })
    }
    case isSwap: {
      txArgs = createMockEthSwapTxArgs({
        buyAmountWei: buyAmount || '',
        buyTokenContractAddress: buyAssetContractAddress || '',
        sellAmountWei: sendApproveOrSellAmount,
        sellTokenContractAddress: sendApproveOrSellAssetContractAddress
      })
    }
  }

  const txBase = coinType === BraveWallet.CoinType.FIL
    ? mockFilSendTransaction
    : deserializeTransaction(
      coinType === BraveWallet.CoinType.ETH
        ? mockTransactionInfo
        : coinType === BraveWallet.CoinType.SOL
          ? mockSolanaTransactionInfo
          : mockTransactionInfo
    )
  return {
    ...(txBase),
    id: `${txBase.id}--${JSON.stringify(arg)}`,
    chainId,
    fromAddress,
    txArgs
  }
}
