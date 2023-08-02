// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  AppsListType,
  BraveWallet,
  CoinType,
  SerializableTransactionInfo,
  SpotPriceRegistry
} from '../../constants/types'
import { NftsPinningStatusType } from '../../page/constants/action_types'

// mocks
import { mockBasicAttentionToken } from '../../stories/mock-data/mock-asset-options'
import { getAssetIdKey } from '../../utils/asset-utils'
import { getPriceIdForToken } from '../../utils/api-utils'


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
        solanaTxData: undefined
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
      groupId: undefined,
      effectiveRecipient: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f'
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
  iconUrls: [],
  coin: CoinType.ETH,
  supportedKeyrings: [BraveWallet.KeyringId.kDefault],
  isEip1559: false
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
  iconUrls: [],
  coin: CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoin],
  isEip1559: false
}

export const mockFilecoinTestnetNetwork: BraveWallet.NetworkInfo = {
  chainId: 't',
  chainName: 'Filecoin Testnet',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [{ url: 'https://mainnet-beta-solana.brave.com/rpc' }],
  blockExplorerUrls: ['https://calibration.filscan.io/tipset/message-detail'],
  symbol: 'FIL',
  symbolName: 'Filecoin',
  decimals: 18,
  iconUrls: [],
  coin: CoinType.FIL,
  supportedKeyrings: [BraveWallet.KeyringId.kFilecoinTestnet],
  isEip1559: false
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
  iconUrls: [],
  coin: CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana],
  isEip1559: false
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
  iconUrls: [],
  coin: CoinType.SOL,
  supportedKeyrings: [BraveWallet.KeyringId.kSolana],
  isEip1559: false
}

export const mockERC20Token: BraveWallet.BlockchainToken = {
  contractAddress: 'mockContractAddress',
  name: 'Dog Coin',
  symbol: 'DOG',
  logo: '',
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

export const mockErc721Token: BraveWallet.BlockchainToken = {
  contractAddress: '0x59468516a8259058bad1ca5f8f4bff190d30e066',
  name: 'Invisible Friends',
  symbol: 'INVSBLE',
  logo: 'https://ipfs.io/ipfs/QmX4nfgA35MiW5APoc4P815hMcH8hAt7edi5H3wXkFm485/2D/2585.gif',
  isErc20: false,
  isErc721: true,
  isErc1155: false,
  isNft: true,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '0x0a19',
  coingeckoId: '',
  coin: CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

export const mockNftPinningStatus: NftsPinningStatusType = {
  [getAssetIdKey(mockErc721Token)]: {
    code: BraveWallet.TokenPinStatusCode.STATUS_PINNED,
    error: undefined
  }
}

export const mockAccount: BraveWallet.AccountInfo = {
  name: 'mockAccountName',
  address: 'mockAddress',
  accountId: {
    coin: CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: 'mockAddress',
    bitcoinAccountIndex: 0,
    uniqueKey: 'mockId'
  },
  hardware: undefined
}

export const mockEthAccountInfo: BraveWallet.AccountInfo = {
  hardware: undefined,
  name: 'mockEthAccountName',
  address: 'mockEthAddress',
  accountId: {
    coin: CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: 'mockEthAddress',
    bitcoinAccountIndex: 0,
    uniqueKey: 'mockEthAddress',
  },
}

export const mockSolanaAccount: BraveWallet.AccountInfo = {
  name: 'MockSolanaAccount',
  address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
  accountId: {
    coin: CoinType.SOL,
    keyringId: BraveWallet.KeyringId.kSolana,
    kind: BraveWallet.AccountKind.kDerived,
    address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
    bitcoinAccountIndex: 0,
    uniqueKey: 'mockId-2'
  },
  hardware: undefined
}

export const mockSolanaAccountInfo: BraveWallet.AccountInfo = {
  name: 'MockSolanaAccount',
  address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
  accountId: {
    coin: CoinType.SOL,
    keyringId: BraveWallet.KeyringId.kSolana,
    kind: BraveWallet.AccountKind.kDerived,
    address: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
    bitcoinAccountIndex: 0,
    uniqueKey: '5sDWP4vCRgDrGsmS1RRuWGRWKo5mhP5wKw8RNqK6zRer',
  },
  hardware: undefined
}

export const mockFilecoinAccount: BraveWallet.AccountInfo = {
  name: 'MockFilecoinAccount',
  address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
  accountId: {
    coin: CoinType.FIL,
    keyringId: BraveWallet.KeyringId.kFilecoinTestnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
    bitcoinAccountIndex: 0,
    uniqueKey: 'mockId-3'
  },
  hardware: undefined
}

export const mockFilecoinAccountInfo: BraveWallet.AccountInfo = {
  name: 'MockFilecoinAccount',
  address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
  accountId: {
    coin: CoinType.FIL,
    keyringId: BraveWallet.KeyringId.kFilecoinTestnet,
    kind: BraveWallet.AccountKind.kDerived,
    address: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
    bitcoinAccountIndex: 0,
    uniqueKey: 't1alebc2ujfh4kuxs5bvzmx5b2w5ixrqrl3ni5rti',
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

export const mockAppsList: AppsListType[] = [
  {
    category: 'category1',
    categoryButtonText: 'categoryButtonText1',
    appList: [
      {
        name: 'foo',
        description: 'description1'
      }
    ] as BraveWallet.AppItem[]
  },
  {
    category: 'category2',
    categoryButtonText: 'categoryButtonText2',
    appList: [
      {
        name: 'bar',
        description: 'description2'
      }
    ] as BraveWallet.AppItem[]
  }
]

export const mockSolDappSignTransactionRequest: BraveWallet.SignTransactionRequest = {
  'originInfo': {
    'originSpec': 'https://f40y4d.csb.app',
    'eTldPlusOne': 'csb.app'
  },
  'id': 0,
  'fromAddress': mockSolanaAccount.address,
  'txData': {
    'ethTxData': undefined,
    'ethTxData1559': undefined,
    'filTxData': undefined,
    'solanaTxData': {
      'recentBlockhash': 'B7Kg79jDm48LMdB4JB2hu82Yfsuz5xYm2cQDBYmKdDSn',
      'lastValidBlockHeight': 0 as unknown as bigint,
      'feePayer': mockSolanaAccount.address,
      'toWalletAddress': '',
      'splTokenMintAddress': '',
      'lamports': 0 as unknown as bigint,
      'amount': 0 as unknown as bigint,
      'txType': 12,
      'instructions': [
        {
          'programId': '11111111111111111111111111111111',
          'accountMetas': [
            {
              'pubkey': mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              'isSigner': true,
              'isWritable': true
            },
            {
              'pubkey': mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              'isSigner': true,
              'isWritable': true
            }
          ],
          'data': [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          'decodedData': undefined
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
      'sendOptions': undefined,
      'signTransactionParam': undefined
    }
  },
  'rawMessage': { bytes: [1], str: undefined },
  'coin': CoinType.SOL,
  'chainId': BraveWallet.SOLANA_MAINNET
}

// BraveWallet.TransactionInfo (selectedPendingTransaction)
export const mockSolDappSignAndSendTransactionRequest: SerializableTransactionInfo = {
  chainId: '0x67',
  id: 'e1eae32d-5bc2-40ac-85e5-2a4a5fbe8a5f',
  fromAddress: mockSolanaAccount.address,
  fromAccountId: mockSolanaAccount.accountId,
  txHash: '',
  txDataUnion: {
    ethTxData: undefined,
    ethTxData1559: undefined,
    filTxData: undefined,
    solanaTxData: {
      recentBlockhash: 'C115cyMDVoGGYNd4r8vFy5qPJEUdoJQQCXMYYKQTQimn',
      lastValidBlockHeight: '0',
      feePayer: mockSolanaAccount.address,
      toWalletAddress: '',
      splTokenMintAddress: '',
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
      signTransactionParam: undefined
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
  groupId: undefined,
  effectiveRecipient: undefined
}

export const mockSolDappSignAllTransactionsRequest: BraveWallet.SignAllTransactionsRequest = {
  'originInfo': {
    'originSpec': 'https://f40y4d.csb.app',
    'eTldPlusOne': 'csb.app'
  },
  'id': 3,
  'fromAddress': mockSolanaAccount.address,
  'txDatas': [
    {
      'ethTxData': undefined,
      'ethTxData1559': undefined,
      'filTxData': undefined,
      'solanaTxData': {
        'recentBlockhash': '8Yq6DGZBh9oEJsCVhUjTqN9kPiLoeYJ7J4n9TnpPYjqW',
        'lastValidBlockHeight': 0 as unknown as bigint,
        'feePayer': mockSolanaAccount.address,
        'toWalletAddress': '',
        'splTokenMintAddress': '',
        'lamports': 0 as unknown as bigint,
        'amount': 0 as unknown as bigint,
        'txType': 12,
        'instructions': [{
          'programId': '11111111111111111111111111111111',
          'accountMetas': [
            {
              'pubkey': mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              'isSigner': true,
              'isWritable': true
            },
            {
              'pubkey': mockSolanaAccount.address,
              addrTableLookupIndex: undefined,
              'isSigner': true,
              'isWritable': true
            }
          ],
          'data': [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
          'decodedData': undefined
        }],
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
        'sendOptions': undefined,
        'signTransactionParam': undefined
      }
    },
    {
      'ethTxData': undefined,
      'ethTxData1559': undefined,
      'filTxData': undefined,
      'solanaTxData': {
        'recentBlockhash': '8Yq6DGZBh9oEJsCVhUjTqN9kPiLoeYJ7J4n9TnpPYjqW',
        'lastValidBlockHeight': 0 as unknown as bigint,
        'feePayer': mockSolanaAccount.address,
        'toWalletAddress': '',
        'splTokenMintAddress': '',
        'lamports': 0 as unknown as bigint,
        'amount': 0 as unknown as bigint,
        'txType': 12,
        'instructions': [
          {
            'programId': '11111111111111111111111111111111',
            'accountMetas': [
              {
                'pubkey': mockSolanaAccount.address,
                addrTableLookupIndex: undefined,
                'isSigner': true,
                'isWritable': true
              },
              {
                'pubkey': mockSolanaAccount.address,
                addrTableLookupIndex: undefined,
                'isSigner': true,
                'isWritable': true
              }
            ],
            'data': [2, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0],
            'decodedData': undefined
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
        'sendOptions': undefined,
        'signTransactionParam': undefined
      }
    }
  ],
  'rawMessages': [{ bytes: [1], str: undefined }],
  'coin': CoinType.SOL,
  'chainId': BraveWallet.SOLANA_MAINNET
}
