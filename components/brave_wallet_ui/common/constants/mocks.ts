// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  SerializableTransactionInfo,
  SpotPriceRegistry
} from '../../constants/types'
import type { TokenBalancesRegistry } from '../slices/entities/token-balance.entity'

// images
import {
  ETHIconUrl,
  FILECOINIconUrl,
  SOLIconUrl
} from '../../assets/network_token_icons/network_token_icons'

// utils
import { getPriceIdForToken } from '../../utils/pricing-utils'

// mocks
import {
  mockAlgorandErc20TokenId,
  mockBasicAttentionToken,
  mockBasicAttentionTokenId,
  mockBinanceCoinErc20TokenId,
  mockBitcoinErc20TokenId,
  mockDaiTokenId,
  mockEthToken,
  mockMoonCatNFT,
  mockSplBasicAttentionTokenId,
  mockSplBat,
  mockSplNft,
  mockSplNftId,
  mockSplUSDCoinId,
  mockUSDCoinId,
  mockZrxErc20TokenId
} from '../../stories/mock-data/mock-asset-options'
import { mockNFTMetadata } from '../../stories/mock-data/mock-nft-metadata'
import { mockEthMainnet } from '../../stories/mock-data/mock-networks'

type EIP1559SerializableTransactionInfo = SerializableTransactionInfo & {
  txDataUnion: { ethTxData1559: BraveWallet.TxData1559 }
}

export const getMockedTransactionInfo =
  (): EIP1559SerializableTransactionInfo => {
    return {
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      id: '1',
      fromAddress: mockEthAccountInfo.address,
      fromAccountId: mockEthAccountInfo.accountId,
      txHash: '',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            to: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
            value: '0x01706a99bf354000', // 103700000000000000 wei (0.1037 ETH)
            // data: new Uint8Array(0),
            data: [] as number[],
            nonce: '0x03',
            gasLimit: '0x5208', // 2100
            gasPrice: '0x22ecb25c00', // 150 Gwei
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: BraveWallet.LOCALHOST_CHAIN_ID,
          maxPriorityFeePerGas: '',
          maxFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: {} as any,
        filTxData: undefined,
        solanaTxData: undefined,
        btcTxData: undefined,
        zecTxData: undefined
      },
      txStatus: BraveWallet.TransactionStatus.Approved,
      txType: BraveWallet.TransactionType.Other,
      txParams: [],
      txArgs: [],
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: {
        originSpec: 'https://brave.com',
        eTldPlusOne: 'brave.com'
      },
      effectiveRecipient: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
      isRetriable: false,
      swapInfo: undefined
    }
  }

export const mockNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x1',
  chainName: 'Ethereum Main Net',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.node.glif.io/rpc/v0' }],
  blockExplorerUrls: ['https://etherscan.io'],
  symbol: 'ETH',
  symbolName: 'Ethereum',
  decimals: 18,
  iconUrls: [ETHIconUrl],
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault]
}

export const mockFilecoinEVMMMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x13a',
  chainName: 'Filecoin EVM Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.node.glif.io/rpc/v1' }],
  blockExplorerUrls: ['https://filfox.info/en/message'],
  symbol: 'FIL',
  symbolName: 'Filcoin',
  decimals: 18,
  iconUrls: [FILECOINIconUrl],
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault]
}

export const mockFilecoinEVMMTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x4cb2f',
  chainName: 'Filecoin EVM Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.calibration.node.glif.io/rpc/v1' }],
  blockExplorerUrls: ['https://calibration.filfox.info/en/message'],
  symbol: 'FIL',
  symbolName: 'Filcoin',
  decimals: 18,
  iconUrls: [FILECOINIconUrl],
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault]
}

export const mockFilecoinMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 'f',
  chainName: 'Filecoin Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://calibration.node.glif.io/rpc/v0' }],
  blockExplorerUrls: ['https://filscan.io/tipset/message-detail'],
  symbol: 'FIL',
  symbolName: 'Filecoin',
  decimals: 18,
  iconUrls: [FILECOINIconUrl],
  coin: BraveWallet.CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoin]
}

export const mockFilecoinTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 't',
  chainName: 'Filecoin Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://solana-mainnet.wallet.brave.com' }],
  blockExplorerUrls: ['https://calibration.filscan.io/tipset/message-detail'],
  symbol: 'FIL',
  symbolName: 'Filecoin',
  decimals: 18,
  iconUrls: [FILECOINIconUrl],
  coin: BraveWallet.CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoinTestnet]
}

export const mockSolanaMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x65',
  chainName: 'Solana Mainnet Beta',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.testnet.solana.com' }],
  blockExplorerUrls: ['https://explorer.solana.com'],
  symbol: 'SOL',
  symbolName: 'Solana',
  decimals: 9,
  iconUrls: [SOLIconUrl],
  coin: BraveWallet.CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana]
}

export const mockSolanaTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: '0x66',
  chainName: 'Solana Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://api.testnet.solana.com' }],
  blockExplorerUrls: ['https://explorer.solana.com?cluster=testnet'],
  symbol: 'SOL',
  symbolName: 'Solana',
  decimals: 9,
  iconUrls: [SOLIconUrl],
  coin: BraveWallet.CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana]
}

export const mockBtcMainnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 'bitcoin_mainnet',
  chainName: 'Bitcoin Mainnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://bitcoin-mainnet.wallet.brave.com/' }],
  blockExplorerUrls: ['https://www.blockchain.com/explorer'],
  symbol: 'BTC',
  symbolName: 'Bitcoin',
  decimals: 8,
  iconUrls: [],
  coin: BraveWallet.CoinType.BTC,
  supportedKeyrings: [BraveWallet.KeyringId.kBitcoin84]
}

export const mockAccount: BraveWallet.AccountInfo = {
  name: 'mockAccountName',
  address: '0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0',
  accountId: {
    coin: BraveWallet.CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0',
    accountIndex: 0,
    uniqueKey: 'unique_key_0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0'
  },
  hardware: undefined
}

export const mockBtcAccount: BraveWallet.AccountInfo = {
  name: 'mockBtcAccountName',
  address: 'bc1q4500000000000000000',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84,
    kind: BraveWallet.AccountKind.kDerived,
    address: 'bc1q4500000000000000000',
    accountIndex: 0,
    uniqueKey: 'unique_key_bc1q4500000000000000000'
  },
  hardware: undefined
}

export const mockZecAccount: BraveWallet.AccountInfo = {
  name: 'mockZecAccountName',
  address: 'zCash-address',
  accountId: {
    coin: BraveWallet.CoinType.ZEC,
    keyringId: BraveWallet.KeyringId.kZCashMainnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 'zCash-address',
    accountIndex: 0,
    uniqueKey: 'unique_key_zCash-address'
  },
  hardware: undefined
}

export const mockEthAccountInfo: BraveWallet.AccountInfo = {
  hardware: undefined,
  name: 'mockEthAccountName',
  address: '0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db',
  accountId: {
    coin: BraveWallet.CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db',
    accountIndex: 0,
    uniqueKey: 'unique_key_0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db'
  }
}

export const mockSolanaAccount: BraveWallet.AccountInfo = {
  name: 'MockSolanaAccount',
  address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRez',
  accountId: {
    coin: BraveWallet.CoinType.SOL,
    keyringId: BraveWallet.KeyringId.kSolana,
    kind: BraveWallet.AccountKind.kDerived,
    address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRez',
    accountIndex: 0,
    uniqueKey: 'unique_key_5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRez'
  },
  hardware: undefined
}

export const mockSolanaAccountInfo: BraveWallet.AccountInfo = {
  name: 'MockSolanaAccount',
  address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
  accountId: {
    coin: BraveWallet.CoinType.SOL,
    keyringId: BraveWallet.KeyringId.kSolana,
    kind: BraveWallet.AccountKind.kDerived,
    address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
    accountIndex: 0,
    uniqueKey: 'unique_key_5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer'
  },
  hardware: undefined
}

export const mockFilecoinAccount: BraveWallet.AccountInfo = {
  name: 'MockFilecoinAccount',
  address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
  accountId: {
    coin: BraveWallet.CoinType.FIL,
    keyringId: BraveWallet.KeyringId.kFilecoinTestnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
    accountIndex: 0,
    uniqueKey: 'unique_key_t1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti'
  },
  hardware: undefined
}

export const mockFilecoinAccountInfo: BraveWallet.AccountInfo = {
  name: 'MockFilecoinAccount',
  address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
  accountId: {
    coin: BraveWallet.CoinType.FIL,
    keyringId: BraveWallet.KeyringId.kFilecoinTestnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
    accountIndex: 0,
    uniqueKey: 'unique_key_t1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti'
  },
  hardware: undefined
}

export const mockBitcoinAccount: BraveWallet.AccountInfo = {
  name: 'MockBitcoinAccount',
  address: '',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: 'unique_key_MockBitcoinAccount'
  },
  hardware: undefined
}

export const mockBitcoinTestAccount: BraveWallet.AccountInfo = {
  name: 'MockBitcoinTestAccount',
  address: '',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84Testnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: 'unique_key_MockBitcoinTestAccount'
  },
  hardware: undefined
}

export const mockBitcoinTestnetAccount: BraveWallet.AccountInfo = {
  name: 'MockBitcoinTestnetAccount',
  address: '',
  accountId: {
    coin: BraveWallet.CoinType.BTC,
    keyringId: BraveWallet.KeyringId.kBitcoin84Testnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: 'unique_key_MockBitcoinTestnetAccount'
  },
  hardware: undefined
}

export const mockSpotPriceRegistry: SpotPriceRegistry = {
  eth: {
    fromAsset: 'ETH',
    price: '4000',
    toAsset: 'mockValue',
    assetTimeframeChange: 'mockValue'
  },
  dog: {
    fromAsset: 'DOG',
    price: '100',
    toAsset: 'mockValue',
    assetTimeframeChange: 'mockValue'
  },
  [getPriceIdForToken(mockBasicAttentionToken)]: {
    fromAsset: mockBasicAttentionToken.symbol,
    price: '0.88',
    toAsset: 'mockValue',
    assetTimeframeChange: 'mockValue'
  }
}

export const mockAddresses: string[] = [
  '0xea674fdde714fd979de3edf0f56aa9716b898ec8',
  '0xdbf41e98f541f19bb044e604d2520f3893eefc79',
  '0xcee177039c99d03a6f74e95bbba2923ceea43ea2'
]

export const mockFilAddresses: string[] = [
  't1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  'f1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  't3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a'
]

export const mockFilInvalilAddresses: string[] = [
  '',
  't1lqarsh4nkg545ilaoqdsbtj4uofplt6sto2ziy',
  'f1lqarsh4nkg545ilaoqdsbtj4uofplt6sto2f6ziy',
  't3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3ju3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'f3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jfpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'a1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  'b1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy',
  'c3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a',
  'd3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2oz4o4tpa4mvigcrayh4a'
]

export const mockSolDappSignTransactionRequest: //
BraveWallet.SignSolTransactionsRequest = {
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app'
  },
  id: 0,
  fromAccountId: mockSolanaAccount.accountId,
  fromAddress: mockSolanaAccount.address,
  txDatas: [
    {
      recentBlockhash: 'B7Kg79jDm48LMdB4JB2hu82Yfsuz5xYm2cQDBYmKdDSn',
      lastValidBlockHeight: 0 as unknown as bigint,
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: 0 as unknown as bigint,
      amount: 0 as unknown as bigint,
      txType: 12,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            }
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: undefined
        }
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111'
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined
    }
  ],
  rawMessages: [[1, 2, 3]],
  chainId: BraveWallet.SOLANA_MAINNET
}

// BraveWallet.TransactionInfo (selectedPendingTransaction)
export const mockSolDappSignAndSendTransactionRequest: //
SerializableTransactionInfo = {
  chainId: '0x67',
  id: 'e1eae32d-5bc2-40ac-85e5-2a4a5fbe8a5f',
  fromAddress: mockSolanaAccount.address,
  fromAccountId: mockSolanaAccount.accountId,
  txHash: '',
  txDataUnion: {
    ethTxData: undefined,
    ethTxData1559: undefined,
    filTxData: undefined,
    btcTxData: undefined,
    zecTxData: undefined,
    solanaTxData: {
      recentBlockhash: 'C115cyMDVoGGYNd4r8vFy5qPJEUdoJQQCXMYYKQTQimn',
      lastValidBlockHeight: '0',
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: '0',
      amount: '0',
      txType: 11,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: { val: 1 },
              isSigner: true,
              isWritable: true
            }
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: {
            accountParams: [
              {
                localizedName: 'From',
                name: BraveWallet.FROM_ACCOUNT
              },
              {
                localizedName: 'To',
                name: BraveWallet.TO_ACCOUNT
              }
            ],
            instructionType: BraveWallet.SolanaSystemInstruction.kTransfer,
            params: [
              {
                localizedName: 'lamports',
                name: BraveWallet.LAMPORTS,
                value: '1',
                type: BraveWallet.SolanaInstructionParamType.kUint64
              }
            ]
          }
        }
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111'
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined
    }
  },
  txStatus: 0,
  txType: 11,
  txParams: [],
  txArgs: [],
  createdTime: { microseconds: 1654540245386000 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app'
  },
  effectiveRecipient: undefined,
  isRetriable: false,
  swapInfo: undefined
}

export const mockSolDappSignAllTransactionsRequest: //
BraveWallet.SignSolTransactionsRequest = {
  originInfo: {
    originSpec: 'https://f40y4d.csb.app',
    eTldPlusOne: 'csb.app'
  },
  id: 3,
  fromAccountId: mockSolanaAccount.accountId,
  fromAddress: mockSolanaAccount.address,
  txDatas: [
    {
      recentBlockhash: '8Yq6DGZBh9oEJsCVhUjTqN9kPiLoeYJ7J4n9TnpPYjqW',
      lastValidBlockHeight: 0 as unknown as bigint,
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: 0 as unknown as bigint,
      amount: 0 as unknown as bigint,
      txType: 12,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            }
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: undefined
        }
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111'
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined
    },
    {
      recentBlockhash: '8Yq6DGZBh9oEJsCVhUjTqN9kPiLoeYJ7J4n9TnpPYjqW',
      lastValidBlockHeight: 0 as unknown as bigint,
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      tokenAddress: '',
      lamports: 0 as unknown as bigint,
      amount: 0 as unknown as bigint,
      txType: 12,
      instructions: [
        {
          programId: '11111111111111111111111111111111',
          accountMetas: [
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            },
            {
              pubkey: mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              isSigner: true,
              isWritable: true
            }
          ],
          data: [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          decodedData: undefined
        }
      ],
      version: BraveWallet.SolanaMessageVersion.kLegacy,
      messageHeader: {
        numRequiredSignatures: 1,
        numReadonlySignedAccounts: 0,
        numReadonlyUnsignedAccounts: 1
      },
      staticAccountKeys: [
        mockSolanaAccount.address,
        '11111111111111111111111111111111'
      ],
      addressTableLookups: [],
      sendOptions: undefined,
      signTransactionParam: undefined,
      feeEstimation: undefined
    }
  ],
  rawMessages: [[1]],
  chainId: BraveWallet.SOLANA_MAINNET
}

//
// EVM Simulations
//
export const mockBlowfishAssetPrice: BraveWallet.BlowfishPrice = {
  dollarValuePerToken: '100',
  lastUpdatedAt: new Date().toUTCString(),
  source: BraveWallet.BlowfishAssetPriceSource.kCoingecko
}

const mockTokenVerificationListNames = [
  'COINGECKO',
  'ZERION',
  'ONE_INCH',
  'UNISWAP',
  'MY_CRYPTO_API',
  'KLEROS_TOKENS'
]

export const BlowfishEVMAssets = {
  mainnetETH: {
    address: '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
    decimals: mockEthMainnet.decimals,
    imageUrl:
      mockEthToken.logo ||
      'https://d1ts37qlq4uz4s.cloudfront.net/evm__evm%3A%3Aethereum__' +
        'evm%3A%3Aethereum%3A%3Amainnet__' +
        '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2.png',
    tokenId: '',
    collection: '',
    lists: [],
    name: mockEthToken.name,
    price: {
      dollarValuePerToken: '1600.92',
      lastUpdatedAt: new Date().toISOString(),
      source: BraveWallet.BlowfishAssetPriceSource.kCoingecko
    },
    symbol: mockEthToken.symbol,
    verified: true
  } as BraveWallet.BlowfishEVMAsset,
  sepoliaLink: {
    address: '0x326c977e6efc84e512bb9c30f76e30c160ed06fb',
    symbol: 'LINK',
    name: 'ChainLink Token',
    decimals: 18,
    verified: false,
    lists: [],
    imageUrl: '',
    tokenId: '',
    collection: '',
    price: mockBlowfishAssetPrice
  } as BraveWallet.BlowfishEVMAsset,
  mainnetDai: {
    address: '0x6b175474e89094c44da98b954eedeac495271d0f',
    symbol: 'DAI',
    name: 'Dai Stablecoin',
    decimals: 18,
    verified: true,
    lists: mockTokenVerificationListNames,
    imageUrl:
      'https://d1ts37qlq4uz4s.cloudfront.net/evm__' +
      'evm%3A%3Aethereum__evm%3A%3Aethereum%3A%3Amainnet__' +
      '0x6b175474e89094c44da98b954eedeac495271d0f.png',
    tokenId: '',
    collection: '',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kCoingecko,
      lastUpdatedAt: '1679331222',
      dollarValuePerToken: '0.99'
    }
  } as BraveWallet.BlowfishEVMAsset,
  mainnetTetherUSD: {
    address: '0xdac17f958d2ee523a2206206994597c13d831ec7',
    name: 'Tether USD',
    decimals: 6,
    lists: mockTokenVerificationListNames,
    symbol: 'USDT',
    verified: true,
    imageUrl:
      'https://d1ts37qlq4uz4s.cloudfront.net' +
      '/evm__evm%3A%3Aethereum__evm%3A%3Aethereum%3A%3Amainnet__' +
      '0xdac17f958d2ee523a2206206994597c13d831ec7.png',
    tokenId: '',
    collection: '',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kCoingecko,
      lastUpdatedAt: '1679331222',
      dollarValuePerToken: '0.99'
    }
  } as BraveWallet.BlowfishEVMAsset,
  moonCat: {
    address: mockMoonCatNFT.contractAddress,
    name: mockMoonCatNFT.name,
    decimals: 0,
    lists: [],
    symbol: mockMoonCatNFT.symbol,
    verified: false,
    imageUrl: '',
    tokenId: mockMoonCatNFT.tokenId,
    collection: 'MoonCat',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kSimplehash,
      lastUpdatedAt: new Date().toUTCString(),
      dollarValuePerToken: '572.50'
    }
  } as BraveWallet.BlowfishEVMAsset,
  pudgyPenguins: {
    address: '0xbd3531da5cf5857e7cfaa92426877b022e612cf8',
    name: 'PudgyPenguins',
    collection: 'PudgyPenguins',
    symbol: 'PPG',
    decimals: 0,
    tokenId: '7238',
    verified: false,
    lists: [],
    imageUrl: '',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kSimplehash,
      lastUpdatedAt: new Date().toUTCString(),
      dollarValuePerToken: '594.99'
    }
  } as BraveWallet.BlowfishEVMAsset,
  bayc: {
    address: '0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d',
    name: 'BoredApeYachtClub',
    collection: 'BoredApeYachtClub',
    symbol: 'BAYC',
    decimals: 0,
    tokenId: '6603',
    verified: false,
    lists: [],
    imageUrl: '',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kSimplehash,
      lastUpdatedAt: new Date().toUTCString(),
      dollarValuePerToken: '7865.43'
    }
  } as BraveWallet.BlowfishEVMAsset,
  corgi: {
    address: '0x51e613727fdd2e0b91b51c3e5427e9440a7957e4',
    tokenId: '1234',
    collection: '',
    symbol: '',
    name: 'Corgi #1234',
    decimals: 0,
    verified: false,
    lists: [],
    imageUrl: '',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kSimplehash,
      lastUpdatedAt: new Date().toUTCString(),
      dollarValuePerToken: '232.43'
    }
  } as BraveWallet.BlowfishEVMAsset
} as const

const sepoliaLinkTransferData: BraveWallet.BlowfishERC20TransferData = {
  counterparty: {
    address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
    kind: BraveWallet.BlowfishEVMAddressKind.kAccount
  },
  // send
  amount: {
    after: '28907865866843658798',
    before: '14453965866843658798'
  },
  asset: BlowfishEVMAssets.sepoliaLink
}

const mockedSimulationWarnings: BraveWallet.BlowfishWarning[] = [
  {
    kind: BraveWallet.BlowfishWarningKind.kApprovalToEOA,
    message: 'APPROVAL_TO_E_O_A',
    severity: BraveWallet.BlowfishWarningSeverity.kCritical
  }
]

const emptyEvmRawInfo: BraveWallet.BlowfishEVMStateChangeRawInfoDataUnion = {
  erc20TransferData: undefined,
  erc1155ApprovalForAllData: undefined,
  erc1155TransferData: undefined,
  erc20ApprovalData: undefined,
  erc721ApprovalData: undefined,
  erc721ApprovalForAllData: undefined,
  erc721TransferData: undefined,
  nativeAssetTransferData: undefined
}

// EVM Events
export const mockedReceiveDaiEvent: BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Receive 1530.81307 DAI',
  rawInfo: {
    kind: BraveWallet.BlowfishEVMRawInfoKind.kErc20Transfer,
    data: {
      ...emptyEvmRawInfo,
      erc20TransferData: {
        amount: {
          after: '557039306766411381864245',
          before: '555508493698012633714742'
        },
        counterparty: {
          address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        asset: BlowfishEVMAssets.mainnetDai
      }
    }
  }
}

export const mockSendEthEvent: BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Send 1 ETH',
  rawInfo: {
    kind: BraveWallet.BlowfishEVMRawInfoKind.kNativeAssetTransfer,
    data: {
      ...emptyEvmRawInfo,
      nativeAssetTransferData: {
        amount: {
          after: '1182957389356504134754',
          before: '1183957389356504134754'
        },
        counterparty: {
          address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        asset: BlowfishEVMAssets.mainnetETH
      }
    }
  }
}

export const mockApproveUsdtEvent: BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Approve to transfer up to 1000 USDT',
  rawInfo: {
    kind: BraveWallet.BlowfishEVMRawInfoKind.kErc20Approval,
    data: {
      ...emptyEvmRawInfo,
      erc20ApprovalData: {
        amount: {
          after: '1000000000',
          before: '0'
        },
        asset: BlowfishEVMAssets.mainnetTetherUSD,
        owner: {
          address: '0xd8da6bf26964af9d7eed9e03e53415d37aa96045',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        spender: {
          address: '0x68b3465833fb72a70ecdf485e0e4c7bd8665fc45',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        }
      }
    }
  }
}

export const mockReceiveNftEvent: BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Receive PudgyPenguins #7238',
  rawInfo: {
    data: {
      ...emptyEvmRawInfo,
      erc721TransferData: {
        amount: {
          after: '1',
          before: '0'
        },
        counterparty: {
          address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        metadata: {
          rawImageUrl:
            'https://cdn.simplehash.com/assets/' +
            '97e1c9e3e9eb21a1114351f9c5c14fe611c94916f360c4eb3aa9263afd8b837b.png'
        },
        asset: BlowfishEVMAssets.pudgyPenguins
      }
    },
    kind: BraveWallet.BlowfishEVMRawInfoKind.kErc721Transfer
  }
}

export const mockApproveBoredApeNftTransferEvent: //
BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Approve to transfer BoredApeYachtClub',
  rawInfo: {
    kind: BraveWallet.BlowfishEVMRawInfoKind.kErc721Approval,
    data: {
      ...emptyEvmRawInfo,
      erc721ApprovalData: {
        amount: {
          after: '1',
          before: '0'
        },
        metadata: {
          rawImageUrl:
            'https://cdn.simplehash.com/assets/beca5f0f88c267276318' +
            'edd8a6019b6b47327f42efd0ba22a3835e77f27732e5.png'
        },
        owner: {
          address: '0xed2ab4948ba6a909a7751dec4f34f303eb8c7236',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        spender: {
          address: '0x1e0049783f008a0085193e00003d00cd54003c71',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        asset: BlowfishEVMAssets.bayc
      }
    }
  }
}

export const mockApproveAllBoredApeNFTsEvent: //
BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Approve to transfer all your BoredApeYachtClub',
  rawInfo: {
    kind: BraveWallet.BlowfishEVMRawInfoKind.kErc721ApprovalForAll,
    data: {
      ...emptyEvmRawInfo,
      erc721ApprovalForAllData: {
        amount: {
          after: '1157920892373161954235709850086879078532699846656405640394',
          before: '0'
        },
        owner: {
          address: '0x38191ca1307ebf67ca1a7caf5346dbd91d882ca6',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        spender: {
          address: '0x1e0049783f008a0085193e00003d00cd54003c71',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        asset: BlowfishEVMAssets.bayc
      }
    }
  }
}

/**
 * - Receive 14.4539 LINK
 * - Approve 10 LINK
 * - Send 1 Pudgy Pengiuns NFT
 * - Receive 1 Bored Apes NFT
 * - Send 1 ETH
 * - Approve 1 Mooncat NFT
 */
export const mockEvmSimulatedResponse: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  warnings: mockedSimulationWarnings,
  error: undefined,
  expectedStateChanges: [
    {
      humanReadableDiff: 'Receive 14.4539 LINK',
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kErc20Transfer,
        data: {
          ...emptyEvmRawInfo,
          erc20TransferData: sepoliaLinkTransferData
        }
      }
    },
    {
      humanReadableDiff: 'Approve 10 LINK',
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kErc20Approval,
        data: {
          ...emptyEvmRawInfo,
          erc20ApprovalData: {
            amount: {
              after: '10000000000000000000',
              before: '1'
            },
            asset: BlowfishEVMAssets.sepoliaLink,
            owner: {
              address: mockAccount.address,
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            spender: {
              address: mockAccount.address,
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            }
          }
        }
      }
    },
    {
      humanReadableDiff: `Send ${
        BlowfishEVMAssets.pudgyPenguins.name //
      } #${BlowfishEVMAssets.pudgyPenguins.tokenId}`,
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kErc721Transfer,
        data: {
          ...emptyEvmRawInfo,
          erc721TransferData: {
            amount: {
              after: '0',
              before: '1'
            },
            counterparty: {
              address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            metadata: {
              rawImageUrl: mockNFTMetadata[0].imageURL || ''
            },
            asset: BlowfishEVMAssets.pudgyPenguins
          }
        }
      }
    },
    {
      humanReadableDiff: `Receive ${
        BlowfishEVMAssets.bayc.symbol //
      } #${BlowfishEVMAssets.bayc.tokenId}`,
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kErc1155Transfer,
        data: {
          ...emptyEvmRawInfo,
          erc1155TransferData: {
            amount: {
              after: '1',
              before: '0'
            },
            counterparty: {
              address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            metadata: {
              rawImageUrl: mockNFTMetadata[0].imageURL || ''
            },
            asset: BlowfishEVMAssets.bayc
          }
        }
      }
    },
    {
      humanReadableDiff: 'Send 1 ETH',
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kNativeAssetTransfer,
        data: {
          ...emptyEvmRawInfo,
          nativeAssetTransferData: {
            amount: {
              after: '0',
              before: '1000000000000000000'
            },
            asset: BlowfishEVMAssets.mainnetETH,
            counterparty: {
              address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            }
          }
        }
      }
    },
    {
      humanReadableDiff: `Approve ${
        BlowfishEVMAssets.moonCat.name //
      } #${BlowfishEVMAssets.moonCat.tokenId}`,
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kErc721Approval,
        data: {
          ...emptyEvmRawInfo,
          erc721ApprovalData: {
            amount: {
              after: '0',
              before: '1'
            },
            metadata: {
              rawImageUrl: mockMoonCatNFT.logo
            },
            owner: {
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount,
              address: mockAccount.address
            },
            spender: {
              address: mockAccount.address,
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            asset: BlowfishEVMAssets.moonCat
          }
        }
      }
    }
  ]
}

/**
 * ERC20 Swap ETH For DAI
 * - Send 1 ETH
 * - Receive 1530.81307 DAI
 */
export const mockSimulatedSwapETHForDAI: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  warnings: [],
  error: undefined,
  expectedStateChanges: [mockedReceiveDaiEvent, mockSendEthEvent]
}

/**
 * ERC20 Approval
 * - Approve to transfer up to 1000 USDT
 */
export const mockEvmSimulatedERC20Approval: BraveWallet.EVMSimulationResponse =
  {
    action: BraveWallet.BlowfishSuggestedAction.kNone,
    warnings: [],
    error: undefined,
    expectedStateChanges: [mockApproveUsdtEvent]
  }

/**
 * Buy An ERC721 NFT With ETH (Simulated)
 * - Receive PudgyPenguins #7238
 * - Send 3.181 ETH
 */
export const mockSimulatedBuyNFTWithETH: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  warnings: [],
  error: undefined,
  expectedStateChanges: [
    mockReceiveNftEvent,
    {
      humanReadableDiff: 'Send 3.181 ETH',
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kNativeAssetTransfer,
        data: {
          ...emptyEvmRawInfo,
          nativeAssetTransferData: {
            amount: {
              after: '998426264937289938488',
              before: '1001607264937289938488'
            },
            counterparty: {
              address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            asset: BlowfishEVMAssets.mainnetETH
          }
        }
      }
    }
  ]
}

/**
 * ERC721 Approve (Simulated)
 * Approve to transfer BoredApeYachtClub
 */
export const mockSimulatedERC721Approve: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  warnings: [],
  error: undefined,
  expectedStateChanges: [mockApproveBoredApeNftTransferEvent]
}

/**
 * Simulated ERC721 Approve For All
 * - Approve to transfer all your BoredApeYachtClub
 */
export const mockERC721ApproveForAllSim: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kWarn,
  warnings: [
    {
      kind: BraveWallet.BlowfishWarningKind.kUnlimitedAllowanceToNfts,
      message:
        'You are allowing this website ' +
        'to withdraw funds from your account in the future',
      severity: BraveWallet.BlowfishWarningSeverity.kWarning
    }
  ],
  error: undefined,
  expectedStateChanges: [mockApproveAllBoredApeNFTsEvent]
}

export const mockReceiveMultiStandardTokenEvent: //
BraveWallet.BlowfishEVMStateChange = {
  humanReadableDiff: 'Receive Corgi',
  rawInfo: {
    kind: BraveWallet.BlowfishEVMRawInfoKind.kErc1155Transfer,
    data: {
      ...emptyEvmRawInfo,
      erc1155TransferData: {
        amount: {
          'after': '1',
          'before': '0'
        },
        counterparty: {
          address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
          kind: BraveWallet.BlowfishEVMAddressKind.kAccount
        },
        metadata: {
          rawImageUrl:
            'https://cdn.simplehash.com/' +
            'assets/4bedd702e7ea8c4a9d04d83302138fa5b63d0cca0f06df9b87bdb09cff253b88.png'
        },
        asset: BlowfishEVMAssets.corgi
      }
    }
  }
}

/**
 * Buy An ERC1155 Token With ETH
 * - Send 0.033 ETH
 * - Receive Corgi
 */
export const mockSimulatedBuyERC1155Token: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  warnings: [],
  error: undefined,
  expectedStateChanges: [
    {
      humanReadableDiff: 'Send 0.033 ETH',
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kNativeAssetTransfer,
        data: {
          ...emptyEvmRawInfo,
          nativeAssetTransferData: {
            amount: {
              after: '71057321770366572',
              before: '104057321770366572'
            },
            counterparty: {
              address: '0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            asset: BlowfishEVMAssets.mainnetETH
          }
        }
      }
    },
    mockReceiveMultiStandardTokenEvent
  ]
}

/**
 * ERC1155 Approve For All (Simulated)
 * - Approve to transfer all your Sandbox's ASSETs
 */
export const mockEvmERC1155ApproveForAll: BraveWallet.EVMSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  warnings: [
    {
      kind: BraveWallet.BlowfishWarningKind.kUnlimitedAllowanceToNfts,
      message:
        'You are allowing this website to withdraw funds ' +
        'from your account in the future',
      severity: BraveWallet.BlowfishWarningSeverity.kWarning
    }
  ],
  error: undefined,
  expectedStateChanges: [
    {
      humanReadableDiff: `Approve to transfer all your Corgi's ASSETs`,
      rawInfo: {
        kind: BraveWallet.BlowfishEVMRawInfoKind.kErc1155ApprovalForAll,
        data: {
          ...emptyEvmRawInfo,
          erc1155ApprovalForAllData: {
            amount: {
              after:
                '1157920892373161954235709850086879078532699846656405640394',
              before: '0'
            },
            owner: {
              address: '0xed2ab4948ba6a909a7751dec4f34f303eb8c7236',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            spender: {
              address: '0x00000000006c3852cbef3e08e8df289169ede581',
              kind: BraveWallet.BlowfishEVMAddressKind.kAccount
            },
            asset: BlowfishEVMAssets.corgi
          }
        }
      }
    }
  ]
}

//
// Solana SVM Simulations
//
const BlowfishSolanaAssets = {
  mainnetSol: {
    name: 'Solana',
    symbol: 'SOL',
    mint: '',
    decimals: 9,
    imageUrl:
      'https://d1ts37qlq4uz4s.cloudfront.net/solana__solana%3A%3Asolana__solana%3A%3Asolana%3A%3Amainnet__So11111111111111111111111111111111111111112.png',
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kCoingecko,
      lastUpdatedAt: new Date().toUTCString(),
      dollarValuePerToken: '98.54'
    },
    metaplexTokenStandard:
      BraveWallet.BlowfishMetaplexTokenStandardKind.kNonFungible
  } satisfies BraveWallet.BlowfishSolanaAsset,
  bat: {
    decimals: mockSplBat.decimals,
    metaplexTokenStandard:
      BraveWallet.BlowfishMetaplexTokenStandardKind.kFungible,
    mint: mockSplBat.contractAddress,
    name: mockSplBat.name,
    symbol: mockSplBat.symbol,
    imageUrl: mockSplBat.logo,
    price: {
      source: BraveWallet.BlowfishAssetPriceSource.kCoingecko,
      lastUpdatedAt: new Date().toUTCString(),
      dollarValuePerToken: '0.54'
    }
  } satisfies BraveWallet.BlowfishSolanaAsset,
  degenNft: {
    price: mockBlowfishAssetPrice,
    decimals: mockSplNft.decimals,
    imageUrl: mockSplNft.logo,
    metaplexTokenStandard:
      BraveWallet.BlowfishMetaplexTokenStandardKind.kNonFungible,
    mint: mockSplNft.tokenId,
    name: mockSplNft.name,
    symbol: mockSplNft.symbol
  } satisfies BraveWallet.BlowfishSolanaAsset
}

const emptySvmRawInfo: BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion = {
  solStakeAuthorityChangeData: undefined,
  solTransferData: undefined,
  splApprovalData: undefined,
  splTransferData: undefined,
  userAccountOwnerChangeData: undefined
}

const mockSendSolEvent: BraveWallet.BlowfishSolanaStateChange = {
  humanReadableDiff: 'Send 0.0005 SOL',
  suggestedColor: BraveWallet.BlowfishSuggestedColor.kCredit,
  rawInfo: {
    kind: BraveWallet.BlowfishSolanaRawInfoKind.kSolTransfer,
    data: {
      ...emptySvmRawInfo,
      solTransferData: {
        asset: BlowfishSolanaAssets.mainnetSol,
        diff: {
          digits: BigInt(500000).toString(),
          sign: BraveWallet.BlowfishDiffSign.kMinus
        }
      }
    }
  }
}

const mockReceiveSolEvent: BraveWallet.BlowfishSolanaStateChange = {
  humanReadableDiff: 'Receive 0.0005 SOL',
  suggestedColor: BraveWallet.BlowfishSuggestedColor.kCredit,
  rawInfo: {
    kind: BraveWallet.BlowfishSolanaRawInfoKind.kSolTransfer,
    data: {
      ...emptySvmRawInfo,
      solTransferData: {
        asset: BlowfishSolanaAssets.mainnetSol,
        diff: {
          digits: BigInt(500000).toString(),
          sign: BraveWallet.BlowfishDiffSign.kPlus
        }
      }
    }
  }
}

export const mockReceiveSolSimulation: BraveWallet.SolanaSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kBlock,
  error: undefined,
  expectedStateChanges: [mockReceiveSolEvent],
  warnings: [
    {
      kind: BraveWallet.BlowfishWarningKind.kApprovalToEOA,
      message: '',
      severity: BraveWallet.BlowfishWarningSeverity.kCritical
    }
  ]
}

export const mockNoChangeSolSimulation: BraveWallet.SolanaSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kBlock,
  error: undefined,
  expectedStateChanges: [],
  warnings: []
}

export const mockSolStakingChangeEvent: BraveWallet.BlowfishSolanaStateChange =
  {
    humanReadableDiff: 'Re-stake 0.05657 SOL',
    suggestedColor: BraveWallet.BlowfishSuggestedColor.kCredit,
    rawInfo: {
      kind: BraveWallet.BlowfishSolanaRawInfoKind.kSolStakeAuthorityChange,
      data: {
        ...emptySvmRawInfo,
        solStakeAuthorityChangeData: {
          currentAuthorities: {
            staker: mockSolanaAccount.address,
            withdrawer: mockSolanaAccount.address
          },
          futureAuthorities: {
            staker: mockSplNft.contractAddress,
            withdrawer: mockSplNft.contractAddress
          },
          asset: BlowfishSolanaAssets.mainnetSol,
          solStaked: BigInt(5657).toString(),
          stakeAccount: mockSolanaAccountInfo.address
        }
      }
    }
  }

export const mockSendSplTokenEvent: BraveWallet.BlowfishSolanaStateChange = {
  humanReadableDiff: 'Send SPL BAT',
  suggestedColor: BraveWallet.BlowfishSuggestedColor.kDebit,
  rawInfo: {
    kind: BraveWallet.BlowfishSolanaRawInfoKind.kSplTransfer,
    data: {
      ...emptySvmRawInfo,
      splTransferData: {
        asset: BlowfishSolanaAssets.bat,
        diff: {
          digits: BigInt(1000000).toString(),
          sign: BraveWallet.BlowfishDiffSign.kMinus
        },
        counterparty: 'sConterpartyAddress'
      }
    }
  }
}

export const mockSendSolNftEvent: BraveWallet.BlowfishSolanaStateChange = {
  humanReadableDiff: 'Send Brave NFT',
  suggestedColor: BraveWallet.BlowfishSuggestedColor.kDebit,
  rawInfo: {
    kind: BraveWallet.BlowfishSolanaRawInfoKind.kSplTransfer,
    data: {
      ...emptySvmRawInfo,
      splTransferData: {
        asset: BlowfishSolanaAssets.degenNft,
        counterparty: 'sCounterpartyAddress',
        diff: {
          digits: BigInt(1).toString(),
          sign: BraveWallet.BlowfishDiffSign.kMinus
        }
      }
    }
  }
}

export const mockSvmSimulationResult: BraveWallet.SolanaSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kNone,
  error: undefined,
  expectedStateChanges: [
    mockSendSolEvent,
    mockReceiveSolEvent,
    mockSendSplTokenEvent,
    mockSendSolNftEvent,
    mockSolStakingChangeEvent
  ],
  warnings: mockedSimulationWarnings
}

export const mockSolStakingChangeSimulation: //
BraveWallet.SolanaSimulationResponse = {
  action: BraveWallet.BlowfishSuggestedAction.kBlock,
  error: undefined,
  expectedStateChanges: [mockSolStakingChangeEvent],
  warnings: []
}

export const mockOnRampCurrency: BraveWallet.OnRampCurrency = {
  currencyCode: 'USD',
  currencyName: 'United States Dollar',
  providers: []
}

export const mockOnRampCurrencies: BraveWallet.OnRampCurrency[] = [
  mockOnRampCurrency,
  {
    currencyCode: 'EUR',
    currencyName: 'Euro',
    providers: []
  },
  {
    currencyCode: 'GBP',
    currencyName: 'British Pound Sterling',
    providers: []
  }
]

export type NativeAssetBalanceRegistry = Record<
  string, // account address
  | Record<
      string, // chainId
      string // balance
    >
  | undefined
>

export const mockNativeBalanceRegistry: NativeAssetBalanceRegistry = {
  [mockAccount.address]: {
    [BraveWallet.BITCOIN_MAINNET]: '0',
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '836',
    [BraveWallet.FILECOIN_MAINNET]: '0',
    [BraveWallet.MAINNET_CHAIN_ID]: '12312',
    [BraveWallet.SOLANA_MAINNET]: '0',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '2322',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '45100002',
    [BraveWallet.AURORA_MAINNET_CHAIN_ID]: '4326',
    [BraveWallet.AVALANCHE_MAINNET_CHAIN_ID]: '345',
    [BraveWallet.BASE_MAINNET_CHAIN_ID]: '56453455',
    [BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID]: '444',
    [BraveWallet.CELO_MAINNET_CHAIN_ID]: '55851',
    [BraveWallet.FANTOM_MAINNET_CHAIN_ID]: '1',
    [BraveWallet.GNOSIS_CHAIN_ID]: '440502',
    [BraveWallet.NEON_EVM_MAINNET_CHAIN_ID]: '222',
    [BraveWallet.OPTIMISM_MAINNET_CHAIN_ID]: '567',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '111',
    [BraveWallet.POLYGON_ZKEVM_CHAIN_ID]: '98094343',
    [BraveWallet.ZK_SYNC_ERA_CHAIN_ID]: '2621',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '0',
    [BraveWallet.LOCALHOST_CHAIN_ID]: '133',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '7798',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '777',
    [BraveWallet.PALM_CHAIN_ID]: '2'
  },
  [mockEthAccountInfo.address]: {
    [BraveWallet.BITCOIN_MAINNET]: '0',
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '22',
    [BraveWallet.FILECOIN_MAINNET]: '3111',
    [BraveWallet.MAINNET_CHAIN_ID]: '33214',
    [BraveWallet.SOLANA_MAINNET]: '0',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '1221',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '251002',
    [BraveWallet.AURORA_MAINNET_CHAIN_ID]: '1111',
    [BraveWallet.AVALANCHE_MAINNET_CHAIN_ID]: '565',
    [BraveWallet.BASE_MAINNET_CHAIN_ID]: '4444',
    [BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID]: '2122',
    [BraveWallet.CELO_MAINNET_CHAIN_ID]: '1',
    [BraveWallet.FANTOM_MAINNET_CHAIN_ID]: '0',
    [BraveWallet.GNOSIS_CHAIN_ID]: '2',
    [BraveWallet.NEON_EVM_MAINNET_CHAIN_ID]: '0',
    [BraveWallet.OPTIMISM_MAINNET_CHAIN_ID]: '2',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '55',
    [BraveWallet.POLYGON_ZKEVM_CHAIN_ID]: '666',
    [BraveWallet.ZK_SYNC_ERA_CHAIN_ID]: '5377',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '1',
    [BraveWallet.LOCALHOST_CHAIN_ID]: '3',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '9',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '727',
    [BraveWallet.PALM_CHAIN_ID]: '1'
  },
  [mockSolanaAccount.address]: {
    [BraveWallet.SOLANA_MAINNET]: '7432'
  },
  [mockSolanaAccountInfo.address]: {
    [BraveWallet.SOLANA_MAINNET]: '45434545435'
  },
  [mockFilecoinAccount.address]: {
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '34598722',
    [BraveWallet.FILECOIN_MAINNET]: '345545',
    [BraveWallet.MAINNET_CHAIN_ID]: '1000',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '1000',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '3000',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '330',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '220',
    [BraveWallet.LOCALHOST_CHAIN_ID]: '11110',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '5550',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '40',
    [BraveWallet.PALM_CHAIN_ID]: '70'
  },
  [mockFilecoinAccountInfo.address]: {
    [BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID]: '2334',
    [BraveWallet.FILECOIN_MAINNET]: '35',
    [BraveWallet.MAINNET_CHAIN_ID]: '220',
    // Secondary Networks
    [BraveWallet.ARBITRUM_MAINNET_CHAIN_ID]: '600',
    [BraveWallet.ARBITRUM_NOVA_CHAIN_ID]: '400',
    [BraveWallet.POLYGON_MAINNET_CHAIN_ID]: '30',
    // Test Networks
    [BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID]: '20',
    [BraveWallet.LOCALHOST_CHAIN_ID]: '110',
    [BraveWallet.SEPOLIA_CHAIN_ID]: '50',
    // Other
    [BraveWallet.GODWOKEN_CHAIN_ID]: '4',
    [BraveWallet.PALM_CHAIN_ID]: '7'
  }
}

export const mockTokenBalanceRegistry: TokenBalancesRegistry = {
  accounts: {
    [mockAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.MAINNET_CHAIN_ID]: {
          tokenBalances: {
            [mockBasicAttentionTokenId]: '111',
            [mockBinanceCoinErc20TokenId]: '222',
            [mockBitcoinErc20TokenId]: '333',
            [mockAlgorandErc20TokenId]: '444',
            [mockZrxErc20TokenId]: '555',
            [mockDaiTokenId]: '666',
            [mockUSDCoinId]: '777'
          }
        }
      }
    },
    [mockEthAccountInfo.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.MAINNET_CHAIN_ID]: {
          tokenBalances: {
            [mockBasicAttentionTokenId]: '11',
            [mockBinanceCoinErc20TokenId]: '22',
            [mockBitcoinErc20TokenId]: '33',
            [mockAlgorandErc20TokenId]: '44',
            [mockZrxErc20TokenId]: '55',
            [mockDaiTokenId]: '66',
            [mockUSDCoinId]: '77'
          }
        }
      }
    },
    [mockSolanaAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.SOLANA_MAINNET]: {
          tokenBalances: {
            [mockSplNftId]: '1',
            [mockSplUSDCoinId]: '14444',
            [mockSplBasicAttentionTokenId]: '99999'
          }
        }
      }
    },
    [mockSolanaAccountInfo.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.SOLANA_MAINNET]: {
          tokenBalances: {
            [mockSplNftId]: '0',
            [mockSplUSDCoinId]: '3333',
            [mockSplBasicAttentionTokenId]: '3421'
          }
        }
      }
    },
    [mockFilecoinAccount.accountId.uniqueKey]: {
      chains: {
        [BraveWallet.FILECOIN_MAINNET]: {
          tokenBalances: {}
        }
      }
    },
    [mockFilecoinAccountInfo.address]: {
      chains: {
        [BraveWallet.FILECOIN_MAINNET]: {
          tokenBalances: {}
        }
      }
    }
  }
}
