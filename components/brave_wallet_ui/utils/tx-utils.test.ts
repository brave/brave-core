// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '../../common/locale'
import {
  getMockedTransactionInfo,
  mockAccount,
  mockNetwork,
  mockSolanaAccount,
} from '../common/constants/mocks'
import { NATIVE_EVM_ASSET_CONTRACT_ADDRESS } from '../common/constants/magics'
import { BraveWallet, SerializableTransactionInfo } from '../constants/types'
import {
  mockBasicAttentionToken,
  mockBitcoinErc20Token,
  mockBtcToken,
  mockErc20TokensList,
  mockEthToken,
  mockFilToken,
  mockMoonCatNFT,
  mockSolToken,
  mockSplBat,
  mockZecToken,
} from '../stories/mock-data/mock-asset-options'
import {
  NetworksRegistry,
  networkEntityAdapter,
} from '../common/slices/entities/network.entity'
import {
  createMockTransactionInfo,
  mockATAInstruction,
  mockBtcSendTransaction,
  mockEthSendTransaction,
  mockFilSendTransaction,
  mockSolanaTransactionInfo,
  mockTransactionInfo,
  mockZecSendTransaction,
} from '../stories/mock-data/mock-transaction-info'
import { mockEthAccount } from '../stories/mock-data/mock-wallet-accounts'
import Amount from './amount'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  findTransactionToken,
  getIsRevokeApprovalTx,
  getIsSolanaAssociatedTokenAccountCreation,
  getTransactionGas,
  getTransactionStatusString,
  getTransactionTypeName,
  isCancelTransaction,
  parseSwapInfo,
  toTxDataUnion,
} from './tx-utils'

describe('Check Transaction Status Strings Value', () => {
  test('Transaction ID 0 should return Unapproved', () => {
    expect(getTransactionStatusString(0)).toEqual(
      'braveWalletTransactionStatusUnapproved',
    )
  })
  test('Transaction ID 1 should return Approved', () => {
    expect(getTransactionStatusString(1)).toEqual(
      'braveWalletTransactionStatusApproved',
    )
  })

  test('Transaction ID 2 should return Rejected', () => {
    expect(getTransactionStatusString(2)).toEqual(
      'braveWalletTransactionStatusRejected',
    )
  })

  test('Transaction ID 3 should return Submitted', () => {
    expect(getTransactionStatusString(3)).toEqual(
      'braveWalletTransactionStatusSubmitted',
    )
  })

  test('Transaction ID 4 should return Confirmed', () => {
    expect(getTransactionStatusString(4)).toEqual(
      'braveWalletTransactionStatusConfirmed',
    )
  })

  test('Transaction ID 5 should return Error', () => {
    expect(getTransactionStatusString(5)).toEqual(
      'braveWalletTransactionStatusError',
    )
  })

  test('Transaction ID 6 should return Dropped', () => {
    expect(getTransactionStatusString(6)).toEqual(
      'braveWalletTransactionStatusDropped',
    )
  })

  test('Transaction ID 7 should return Signed', () => {
    expect(getTransactionStatusString(7)).toEqual(
      'braveWalletTransactionStatusSigned',
    )
  })

  // Follow up issue to fix test via https://github.com/brave/brave-browser/issues/43583

  // test('Transaction ID 8 should return an empty string', () => {
  //   expect(getTransactionStatusString(8)).toEqual('')
  // })
})

describe('getTransactionGas()', () => {
  it('should get the gas values of a FIL transaction', () => {
    const txGas = getTransactionGas(mockFilSendTransaction)

    const { filTxData } = mockFilSendTransaction.txDataUnion

    expect(txGas.maxFeePerGas).toBe(filTxData.gasFeeCap || '')
    expect(txGas.maxPriorityFeePerGas).toBe(filTxData.gasPremium)
    expect(txGas.gasPrice).toBe(
      new Amount(filTxData.gasFeeCap)
        .minus(filTxData.gasPremium)
        .value?.toString() || '',
    )
  })
  it('should get the gas values of an EVM transaction', () => {
    const txGas = getTransactionGas(mockTransactionInfo)

    const { ethTxData1559 } = mockTransactionInfo.txDataUnion

    expect(txGas.maxFeePerGas).toBe(ethTxData1559?.maxFeePerGas || '')
    expect(txGas.maxPriorityFeePerGas).toBe(
      ethTxData1559?.maxPriorityFeePerGas || '',
    )
    expect(txGas.gasPrice).toBe(ethTxData1559?.baseData.gasPrice || '')
  })
})

describe('parseSwapInfo', () => {
  const createMockNetworksRegistry = (): NetworksRegistry => {
    const sourceNetworkId = networkEntityAdapter.selectId({
      chainId: mockBasicAttentionToken.chainId,
      coin: mockBasicAttentionToken.coin,
    }) as string
    const destinationNetworkId = networkEntityAdapter.selectId({
      chainId: mockBitcoinErc20Token.chainId,
      coin: mockBitcoinErc20Token.coin,
    }) as string

    return {
      ...networkEntityAdapter.getInitialState(),
      entities: {
        [sourceNetworkId]: mockNetwork,
        [destinationNetworkId]: mockNetwork,
      },
      ids: [sourceNetworkId, destinationNetworkId],
      hiddenIds: [],
      hiddenIdsByCoinType: {},
      visibleIdsByCoinType: {},
      mainnetIds: [],
      testnetIds: [],
      offRampIds: [],
      visibleIds: [sourceNetworkId, destinationNetworkId],
    }
  }

  it('should parse swap info with new structure (source/destination)', () => {
    const networksRegistry = createMockNetworksRegistry()
    const {
      sourceToken,
      sourceAmount,
      destinationToken,
      destinationAmount,
      destinationAmountMin,
      destinationAddress,
      provider,
    } = parseSwapInfo({
      tokensList: mockErc20TokensList,
      swapInfo: {
        sourceCoin: mockBasicAttentionToken.coin,
        sourceChainId: mockBasicAttentionToken.chainId,
        sourceTokenAddress: mockBasicAttentionToken.contractAddress,
        sourceAmount: '1000000000000000000', // 1 token with 18 decimals
        destinationCoin: mockBitcoinErc20Token.coin,
        destinationChainId: mockBitcoinErc20Token.chainId,
        destinationTokenAddress: mockBitcoinErc20Token.contractAddress,
        destinationAmount: '2000000000000000000', // 2 tokens
        destinationAmountMin: '1900000000000000000', // 1.9 tokens
        recipient: '0x1234567890123456789012345678901234567890',
        provider: BraveWallet.SwapProvider.kZeroEx,
      },
      networksRegistry,
    })

    expect(sourceToken).toBeDefined()
    expect(destinationToken).toBeDefined()
    expect(sourceToken?.contractAddress).toBe(
      mockBasicAttentionToken.contractAddress,
    )
    expect(destinationToken?.contractAddress).toBe(
      mockBitcoinErc20Token.contractAddress,
    )
    expect(sourceAmount.format()).toEqual('1000000000000000000')
    expect(
      sourceAmount.divideByDecimals(mockBasicAttentionToken.decimals).format(6),
    ).toEqual('1')
    expect(destinationAmount.format()).toEqual('2000000000000000000')
    expect(destinationAmount.divideByDecimals(18).format(6)).toEqual('2')
    expect(destinationAmountMin.divideByDecimals(18).format(6)).toEqual('1.9')
    expect(destinationAddress).toBe(
      '0x1234567890123456789012345678901234567890',
    )
    expect(provider).toBe(BraveWallet.SwapProvider.kZeroEx)
  })

  it('should handle undefined swapInfo', () => {
    const networksRegistry = createMockNetworksRegistry()
    const {
      sourceToken,
      sourceAmount,
      destinationToken,
      destinationAmount,
      destinationAmountMin,
      destinationAddress,
      provider,
    } = parseSwapInfo({
      tokensList: mockErc20TokensList,
      swapInfo: undefined,
      networksRegistry,
    })

    expect(sourceToken).toBeUndefined()
    expect(destinationToken).toBeUndefined()
    expect(sourceAmount.format()).toEqual('')
    expect(destinationAmount.format()).toEqual('')
    expect(destinationAmountMin.format()).toEqual('')
    expect(destinationAddress).toBe('')
    expect(provider).toBeUndefined()
  })

  it('should handle native assets correctly', () => {
    const networksRegistry = createMockNetworksRegistry()
    const {
      sourceToken,
      sourceAmount,
      destinationToken,
      destinationAmount,
      destinationAmountMin,
      destinationAddress,
      provider,
    } = parseSwapInfo({
      tokensList: mockErc20TokensList,
      swapInfo: {
        sourceCoin: mockBasicAttentionToken.coin,
        sourceChainId: mockBasicAttentionToken.chainId,
        sourceTokenAddress: NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
        sourceAmount: '1000000000000000000', // 1 token with 18 decimals
        destinationCoin: mockBitcoinErc20Token.coin,
        destinationChainId: mockBitcoinErc20Token.chainId,
        destinationTokenAddress: NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
        destinationAmount: '2000000000000000000', // 2 tokens
        destinationAmountMin: '',
        recipient: '',
        provider: BraveWallet.SwapProvider.kZeroEx,
      },
      networksRegistry,
    })

    expect(sourceToken).toBeDefined()
    expect(destinationToken).toBeDefined()
    expect(sourceToken?.contractAddress).toBe('')
    expect(destinationToken?.contractAddress).toBe('')
    expect(sourceAmount.format()).toEqual('1000000000000000000')
    expect(destinationAmount.format()).toEqual('2000000000000000000')
    expect(destinationAmountMin.format()).toEqual('')
    expect(destinationAddress).toBe('')
    expect(provider).toBe(BraveWallet.SwapProvider.kZeroEx)
  })
})

describe('check for insufficient funds errors', () => {
  describe.each([
    ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
    ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
    ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
    [
      'ERC721SafeTransferFrom',
      BraveWallet.TransactionType.ERC721SafeTransferFrom,
    ],
    ['ETHSend', BraveWallet.TransactionType.ETHSend],
    ['Other', BraveWallet.TransactionType.Other],
  ])('%s', (_, txType) => {
    it('should correctly indicate when funds are insufficient for gas', () => {
      const mockTransactionInfo = getMockedTransactionInfo()

      /**
       * Account balance: 0.001 ETH
       * Transaction value: 0 ETH
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       */
      const nativeBalanceRegistry = {
        [mockTransactionInfo.chainId]: '1000000000000000', // 0.001 ETH
      }

      const accountNativeBalance =
        nativeBalanceRegistry[mockTransactionInfo.chainId]

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance,
        gasFee: '3150000000000000', // 0.00315 ETH
      })

      expect(insufficientFundsForGasError).toBeTruthy()
    })

    it('should be false when funds are sufficient for gas', () => {
      /**
       * Account balance: 1 ETH
       * Transaction value: 0 ETH
       */
      const nativeBalanceRegistry = {
        '0x1': '1000000000000000000', // 1 ETH
      }
      const tokenBalanceRegistry = {
        '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346',
        '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69': '450346',
      }
      /**
       * Gas fee: 0.00315 ETH
       */
      const gasFee = '3150000000000000' // 0.00315 ETH
      const mockTxData: BraveWallet.TxData1559 =
        getMockedTransactionInfo().txDataUnion.ethTxData1559

      const mockTransactionInfo = {
        ...getMockedTransactionInfo(),
        txArgs: [
          BraveWallet.TransactionType.ERC20Approve,
          BraveWallet.TransactionType.ERC20Transfer,
        ].includes(txType)
          ? ['mockRecipient', '0x0']
          : ['mockOwner', 'mockRecipient', 'mockTokenID'],
        txType,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          btcTxData: undefined,
          ethTxData1559: {
            ...mockTxData,
            baseData: {
              ...mockTxData.baseData,
              value: '0x0',
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00', // 150 Gwei
            },
          },
        },
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance:
          nativeBalanceRegistry[mockTransactionInfo.chainId] || '',
        gasFee,
      })

      const token = findTransactionToken(
        mockTransactionInfo,
        mockErc20TokensList,
      )

      const { sourceAmount, sourceToken } = parseSwapInfo({
        swapInfo: mockTransactionInfo.swapInfo,
        tokensList: mockErc20TokensList,
        networksRegistry: undefined,
      })

      const accountNativeBalance =
        nativeBalanceRegistry[mockTransactionInfo.chainId]

      const sourceTokenBalance =
        tokenBalanceRegistry[sourceToken?.contractAddress ?? '']

      const tokenBalance = tokenBalanceRegistry[token?.contractAddress ?? '']

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance,
        accountTokenBalance: tokenBalance || '',
        gasFee,
        sourceAmount,
        sourceTokenBalance: sourceTokenBalance || '',
        tx: mockTransactionInfo,
        txAccount: mockEthAccount,
      })

      expect(insufficientFundsForGasError).toBeFalsy()
      expect(insufficientFundsError).toBeFalsy()
    })
  })

  describe.each([
    ['ETHSend', BraveWallet.TransactionType.ETHSend],
    ['Other', BraveWallet.TransactionType.Other],
  ])('%s', (_, txType) => {
    const mockTransactionInfo = getMockedTransactionInfo()
    const transactionInfo: SerializableTransactionInfo = {
      ...mockTransactionInfo,
      txType,
      txDataUnion: {
        ethTxData: {} as any,
        filTxData: undefined,
        solanaTxData: undefined,
        btcTxData: undefined,
        zecTxData: undefined,
        ethTxData1559: {
          ...mockTransactionInfo.txDataUnion.ethTxData1559,
          baseData: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
            value: '0xde0b6b3a7640000', // 1 ETH
            gasLimit: '0x5208', // 21000
            gasPrice: '0x22ecb25c00', // 150 Gwei
          },
        },
      },
    }

    it('should be true when funds are insufficient for send amount', () => {
      /**
       * Account balance: 0.004 ETH
       * Transaction value: 1 ETH
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, but not for send amount.
       */
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        [transactionInfo.chainId]: '4000000000000000', // 0.004 ETH
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
        gasFee,
      })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance:
          nativeBalanceRegistry[transactionInfo.chainId] || '',
        accountTokenBalance: '',
        gasFee,
        sourceAmount: Amount.empty(),
        sourceTokenBalance: '',
        tx: transactionInfo,
        txAccount: mockEthAccount,
      })

      expect(insufficientFundsError).toBeTruthy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })

    it('should be false when funds are sufficient for send amount', () => {
      /**
       * Account balance: 1.00315 ETH
       * Transaction value: 1 ETH
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, and for send amount.
       */
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        '0x1': '1003150000000000000', // 1.00315 ETH
      }
      const accountNativeBalance =
        nativeBalanceRegistry[mockTransactionInfo.chainId] || ''

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance,
        gasFee,
      })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance,
        accountTokenBalance: '',
        gasFee,
        sourceAmount: Amount.empty(),
        sourceTokenBalance: '',
        tx: mockTransactionInfo,
        txAccount: mockEthAccount,
      })

      expect(insufficientFundsError).toBeFalsy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })
  })

  describe('ERC20Transfer', () => {
    it('should be true when funds are insufficient for send amount', () => {
      /**
       * Account ETH balance: 0.00315 ETH
       * Account Token balance: 0.99
       * Transaction value: 1 token
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, but not for send amount.
       */
      const mockTransactionInfo = getMockedTransactionInfo()
      const transferredToken = mockErc20TokensList[0]
      const sendAmount = new Amount('1')
        .multiplyByDecimals(transferredToken.decimals)
        .toHex() // 1 full Token
      const transactionInfo: SerializableTransactionInfo = {
        ...mockTransactionInfo,
        // (address recipient, uint256 amount)
        txArgs: ['mockRecipient', sendAmount],
        txType: BraveWallet.TransactionType.ERC20Transfer,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          btcTxData: undefined,
          zecTxData: undefined,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559,
            baseData: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
              to: transferredToken.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00', // 150 Gwei
            },
          },
        },
      }
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        [transactionInfo.chainId]: '1003150000000000000', // 1.00315 ETH
      }
      const tokenBalanceRegistry = {
        [transferredToken.contractAddress]: new Amount('0.99')
          .multiplyByDecimals(transferredToken.decimals)
          .toHex(), // less than 1 full Token
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance:
          nativeBalanceRegistry[transactionInfo.chainId] || '',
        gasFee,
      })

      const token = findTransactionToken(transactionInfo, mockErc20TokensList)

      const { sourceAmount, sourceToken } = parseSwapInfo({
        swapInfo: transactionInfo.swapInfo,
        tokensList: mockErc20TokensList,
        networksRegistry: undefined,
      })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance:
          nativeBalanceRegistry[transactionInfo.chainId] || '',
        accountTokenBalance: tokenBalanceRegistry[token?.contractAddress ?? ''],
        gasFee,
        sourceAmount,
        sourceTokenBalance:
          tokenBalanceRegistry[sourceToken?.contractAddress ?? ''],
        tx: transactionInfo,
        txAccount: mockEthAccount,
      })

      expect(nativeBalanceRegistry[transactionInfo.chainId]).toBeTruthy()
      expect(token).toBeTruthy()
      expect(tokenBalanceRegistry[token?.contractAddress ?? '']).toBeTruthy()
      expect(insufficientFundsError).toBeTruthy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })

    it('should be false when funds are sufficient for send amount', () => {
      /**
       * Account ETH balance: 0.00315 ETH
       * Account Token balance: 2
       * Transaction value: 0.5 tokens
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, but not for send amount.
       */
      const mockTransactionInfo = getMockedTransactionInfo()
      const transferredToken = mockErc20TokensList[0]
      const sendAmount = new Amount('0.5')
        .multiplyByDecimals(transferredToken.decimals)
        .toHex() // half of a full token

      const transactionInfo: SerializableTransactionInfo = {
        ...mockTransactionInfo,
        // (address recipient, uint256 amount)
        txArgs: ['mockRecipient', sendAmount],
        txType: BraveWallet.TransactionType.ERC20Transfer,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          btcTxData: undefined,
          zecTxData: undefined,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559,
            baseData: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
              to: transferredToken.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00', // 150 Gwei
            },
          },
        },
      }
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        [transactionInfo.chainId]: '1003150000000000000', // 1.00315 ETH
      }
      const tokenBalanceRegistry = {
        [transferredToken.contractAddress]: new Amount('2')
          .multiplyByDecimals(transferredToken.decimals)
          .toHex(), // 2 full Tokens
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance:
          nativeBalanceRegistry[transactionInfo.chainId] || '',
        gasFee,
      })

      const token = findTransactionToken(transactionInfo, mockErc20TokensList)

      const { sourceAmount, sourceToken } = parseSwapInfo({
        swapInfo: transactionInfo.swapInfo,
        tokensList: mockErc20TokensList,
        networksRegistry: undefined,
      })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance:
          nativeBalanceRegistry[transactionInfo.chainId] || '',
        accountTokenBalance: tokenBalanceRegistry[token?.contractAddress ?? ''],
        gasFee,
        sourceAmount,
        sourceTokenBalance:
          tokenBalanceRegistry[sourceToken?.contractAddress ?? ''],
        tx: transactionInfo,
        txAccount: mockEthAccount,
      })

      expect(insufficientFundsError).toBeFalsy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })
  })
})

describe('toTxDataUnion', () => {
  test('works', () => {
    const filTxData: BraveWallet.FilTxData = {
      nonce: '',
      gasPremium: '',
      gasFeeCap: '',
      gasLimit: '',
      maxFee: '0',
      to: 'to',
      value: 'value',
    }

    const union = toTxDataUnion({ filTxData: filTxData })

    expect(Object.keys(union).length).toBe(1)
    expect(union.filTxData).toBe(filTxData)
    expect(union.ethTxData).toBe(undefined)
    expect(union.ethTxData1559).toBe(undefined)
    expect(union.solanaTxData).toBe(undefined)
    expect(union.btcTxData).toBe(undefined)
    expect(union.zecTxData).toBe(undefined)
  })
})

describe('getIsRevokeApprovalTx', () => {
  test('correctly detects revocations', () => {
    expect(
      getIsRevokeApprovalTx({
        ...mockTransactionInfo,
        txType: BraveWallet.TransactionType.ERC20Approve,
        txArgs: [
          mockEthAccount.address, // spender
          '0', // amount
        ],
      }),
    ).toBe(true)
  })
  test('correctly detects non-revocation approvals', () => {
    expect(
      getIsRevokeApprovalTx({
        ...mockTransactionInfo,
        txType: BraveWallet.TransactionType.ERC20Approve,
        txArgs: [
          mockEthAccount.address, // spender
          '10', // amount
        ],
      }),
    ).toBe(false)
  })
})

describe('findTransactionToken', () => {
  describe('Native asset Transfers', () => {
    it(
      'should detect SOL '
        + 'as the token for Solana System transfer transactions',
      () => {
        expect(
          findTransactionToken(mockSolanaTransactionInfo, [mockSolToken])
            ?.symbol,
        ).toBe('SOL')
      },
    )

    it('should detect FIL as the token for Filecoin send transactions', () => {
      expect(
        findTransactionToken(mockFilSendTransaction, [mockFilToken])?.symbol,
      ).toBe('FIL')
    })

    it('should detect ETH as the token for Ethereum send transactions', () => {
      expect(
        findTransactionToken(mockEthSendTransaction, [mockEthToken])?.symbol,
      ).toBe('ETH')
    })

    it('should detect BTC as the token for Bitcoin transactions', () => {
      expect(
        findTransactionToken(mockBtcSendTransaction, [mockBtcToken])?.symbol,
      ).toBe('BTC')
    })

    it('should detect ZEC as the token for ZCash transactions', () => {
      expect(
        findTransactionToken(mockZecSendTransaction, [mockZecToken])?.symbol,
      ).toBe('ZEC')
    })
  })

  describe('Token Transfers', () => {
    it('should detect sent ERC20 tokens', () => {
      expect(
        findTransactionToken(
          createMockTransactionInfo({
            chainId: BraveWallet.MAINNET_CHAIN_ID,
            coinType: BraveWallet.CoinType.ETH,
            fromAccount: mockEthAccount,
            sendApproveOrSellAmount: '1000',
            sendApproveOrSellAssetContractAddress:
              mockBasicAttentionToken.contractAddress,
            toAddress: mockAccount.address,
            isERC20Send: true,
          }),
          [mockEthToken, mockBasicAttentionToken],
        )?.symbol,
      ).toBe('BAT')
    })

    it('should detect sent ERC721 tokens', () => {
      expect(
        findTransactionToken(
          createMockTransactionInfo({
            chainId: BraveWallet.MAINNET_CHAIN_ID,
            coinType: BraveWallet.CoinType.ETH,
            fromAccount: mockEthAccount,
            sendApproveOrSellAmount: '1000',
            sendApproveOrSellAssetContractAddress:
              mockMoonCatNFT.contractAddress,
            toAddress: mockAccount.address,
            isERC721Send: true,
          }),
          [mockEthToken, mockMoonCatNFT],
        )?.symbol,
      ).toBe('AMC')
    })

    it('should detect sent SPL tokens', () => {
      expect(
        findTransactionToken(
          createMockTransactionInfo({
            chainId: BraveWallet.SOLANA_MAINNET,
            coinType: BraveWallet.CoinType.SOL,
            fromAccount: mockSolanaAccount,
            sendApproveOrSellAmount: '1000',
            sendApproveOrSellAssetContractAddress: mockSplBat.contractAddress,
            toAddress: mockSolanaAccount.address,
          }),
          [mockSolToken, mockSplBat],
        )?.symbol,
      ).toBe('BAT')
    })
  })
})

describe('getTransactionTypeName', () => {
  test.each([
    {
      txType: BraveWallet.TransactionType.ERC1155SafeTransferFrom,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSafeTransferFrom',
      ),
    },

    {
      txType: BraveWallet.TransactionType.ERC20Approve,
      expectedString: getLocale('braveWalletTransactionTypeNameErc20Approve'),
    },

    {
      txType: BraveWallet.TransactionType.ERC20Transfer,
      expectedString: getLocale('braveWalletTransactionTypeNameTokenTransfer'),
    },

    {
      txType: BraveWallet.TransactionType.ERC721SafeTransferFrom,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSafeTransferFrom',
      ),
    },

    {
      txType: BraveWallet.TransactionType.ERC721TransferFrom,
      expectedString: getLocale('braveWalletTransactionTypeNameNftTransfer'),
    },

    {
      txType: BraveWallet.TransactionType.ETHFilForwarderTransfer,
      expectedString: getLocale('braveWalletTransactionTypeNameForwardFil'),
    },

    {
      txType: BraveWallet.TransactionType.ETHSend,
      expectedString: getLocale('braveWalletTransactionIntentSend').replace(
        '$1',
        'ETH',
      ),
    },

    {
      txType: BraveWallet.TransactionType.ETHSwap,
      expectedString: getLocale('braveWalletSwap'),
    },

    {
      txType: BraveWallet.TransactionType.Other,
      expectedString: getLocale('braveWalletTransactionTypeNameOther'),
    },

    {
      txType: BraveWallet.TransactionType.SolanaCompressedNftTransfer,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameCompressedNftTransfer',
      ),
    },

    {
      txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSignAndSendDappTransaction',
      ),
    },

    {
      txType: BraveWallet.TransactionType.SolanaDappSignTransaction,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSignDappTransaction',
      ),
    },

    {
      txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      expectedString: getLocale('braveWalletTransactionTypeNameTokenTransfer'),
    },

    {
      txType:
        BraveWallet.TransactionType
          .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSplTokenTransferWithAssociatedTokenAccountCreation',
      ),
    },

    {
      txType: BraveWallet.TransactionType.SolanaSwap,
      expectedString: getLocale('braveWalletSwap'),
    },

    {
      txType: BraveWallet.TransactionType.SolanaSystemTransfer,
      expectedString: getLocale('braveWalletTransactionIntentSend').replace(
        '$1',
        'SOL',
      ),
    },
  ])(
    'renders the correct localized function name per tx type',
    ({ expectedString, txType }) => {
      expect(getTransactionTypeName(txType)).toBe(expectedString)
    },
  )

  test('should return "Other" for unknown tx type', () => {
    expect(getTransactionTypeName(999)).toBe(
      getLocale('braveWalletTransactionTypeNameOther'),
    )
  })
})

describe('Test getIsSolanaAssociatedTokenAccountCreation', () => {
  it('should return true for ATACreation transaction', () => {
    expect(
      getIsSolanaAssociatedTokenAccountCreation({
        ...mockSolanaTransactionInfo,
        txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
        txDataUnion: {
          solanaTxData: {
            ...mockSolanaTransactionInfo.txDataUnion.solanaTxData,
            staticAccountKeys: [BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID],
            instructions: [mockATAInstruction],
          },
        },
      }),
    ).toBe(true)
  })

  it('should return false for ATACreation with different payer', () => {
    expect(
      getIsSolanaAssociatedTokenAccountCreation({
        ...mockSolanaTransactionInfo,
        fromAccountId: {
          ...mockSolanaAccount.accountId,
          address: 'different_address',
        },
        txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
        txDataUnion: {
          solanaTxData: {
            ...mockSolanaTransactionInfo.txDataUnion.solanaTxData,
            staticAccountKeys: [BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID],
            instructions: [mockATAInstruction],
          },
        },
      }),
    ).toBe(false)
  })

  it('should return false for a non ATACreation program id', () => {
    expect(
      getIsSolanaAssociatedTokenAccountCreation({
        ...mockSolanaTransactionInfo,
        txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
        txDataUnion: {
          solanaTxData: {
            ...mockSolanaTransactionInfo.txDataUnion.solanaTxData,
            staticAccountKeys: [BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID],
            instructions: [
              {
                ...mockATAInstruction,
                programId: 'different_program_id',
              },
            ],
          },
        },
      }),
    ).toBe(false)
  })

  it('should return false for a ATACreation transaction with data', () => {
    expect(
      getIsSolanaAssociatedTokenAccountCreation({
        ...mockSolanaTransactionInfo,
        txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
        txDataUnion: {
          solanaTxData: {
            ...mockSolanaTransactionInfo.txDataUnion.solanaTxData,
            staticAccountKeys: [BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID],
            instructions: [
              {
                ...mockATAInstruction,
                data: [1],
              },
            ],
          },
        },
      }),
    ).toBe(false)
  })

  it('should return false for a ATACreation with instructions', () => {
    expect(
      getIsSolanaAssociatedTokenAccountCreation({
        ...mockSolanaTransactionInfo,
        txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
        txDataUnion: {
          solanaTxData: {
            ...mockSolanaTransactionInfo.txDataUnion.solanaTxData,
            staticAccountKeys: [BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID],
            instructions: [
              {
                ...mockATAInstruction,
              },
              {
                ...mockATAInstruction,
                accountMetas: [
                  {
                    pubkey: BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID,
                    isSigner: true,
                    isWritable: true,
                    addrTableLookupIndex: undefined,
                  },
                ],
                programId: BraveWallet.SOLANA_STAKE_PROGRAM_ID,
              },
            ],
          },
        },
      }),
    ).toBe(false)
  })
})

describe('isCancelTransaction', () => {
  const mockFromAddress = '0x1234567890123456789012345678901234567890'
  const mockToAddress = '0x0987654321098765432109876543210987654321'

  const mockEthSendTestTransaction = {
    ...mockEthSendTransaction,
    txType: BraveWallet.TransactionType.ETHSend,
    fromAccountId: { address: mockFromAddress } as BraveWallet.AccountId,
  }

  const mockEthTXData = {
    to: mockFromAddress,
    value: '',
    data: [],
    nonce: '',
    gasPrice: '',
    gasLimit: '',
    signOnly: false,
    signedTransaction: '',
  }

  test('should return false for non-ETH transactions', () => {
    const solanaTx = {
      ...mockSolanaTransactionInfo,
      txType: BraveWallet.TransactionType.SolanaSystemTransfer,
    }
    expect(isCancelTransaction(solanaTx)).toBe(false)
  })

  test('should return false for non-ETHSend transactions', () => {
    const erc20Tx = {
      ...mockEthSendTransaction,
      txType: BraveWallet.TransactionType.ERC20Transfer,
    }
    expect(isCancelTransaction(erc20Tx)).toBe(false)
  })

  test('should return false for ETH send with non-zero value', () => {
    const ethSendTx = {
      ...mockEthSendTestTransaction,
      txDataUnion: {
        ethTxData: { ...mockEthTXData, value: '1000000000000000000' },
      },
    }
    expect(isCancelTransaction(ethSendTx)).toBe(false)
  })

  test('should return false for ETH send with data', () => {
    const ethSendTx = {
      ...mockEthSendTestTransaction,
      txDataUnion: {
        ethTxData: {
          ...mockEthTXData,
          value: '0',
          data: [1, 2, 3, 4], // non-empty data
        },
      },
    }
    expect(isCancelTransaction(ethSendTx)).toBe(false)
  })

  test('should return false for ETH send to different address', () => {
    const ethSendTx = {
      ...mockEthSendTestTransaction,
      txDataUnion: {
        ethTxData: {
          ...mockEthTXData,
          to: mockToAddress, // different address
          value: '0',
        },
      },
    }
    expect(isCancelTransaction(ethSendTx)).toBe(false)
  })

  test(
    'should return false for cancel transaction (legacy)'
      + 'with different nonce',
    () => {
      const cancelTx = {
        ...mockEthSendTestTransaction,
        txDataUnion: {
          ethTxData1559: undefined,
          ethTxData: {
            ...mockEthTXData,
            value: '0',
            nonce: '2',
          },
        },
      }
      const submittedTx = {
        ...mockEthSendTestTransaction,
        txDataUnion: {
          ethTxData1559: undefined,
          ethTxData: {
            ...mockEthTXData,
            value: '0',
            nonce: '1',
          },
        },
        txStatus: BraveWallet.TransactionStatus.Submitted,
        id: 'submitted-tx',
      }
      expect(isCancelTransaction(cancelTx, [submittedTx])).toBe(false)
    },
  )

  test(
    'should return false for cancel transaction (EIP1559)'
      + 'with different nonce',
    () => {
      const cancelTx = {
        ...mockEthSendTestTransaction,
        txDataUnion: {
          ethTxData1559: {
            baseData: {
              to: mockFromAddress, // same as from
              value: '0',
              data: [],
              nonce: '4',
              gasPrice: '',
              gasLimit: '',
              signOnly: false,
              signedTransaction: '',
            },
            chainId: '0x1',
            maxPriorityFeePerGas: '0x0',
            maxFeePerGas: '0x0',
            gasEstimation: undefined,
          },
        },
      }
      const submittedTx = {
        ...mockEthSendTestTransaction,
        txDataUnion: {
          ethTxData1559: {
            baseData: {
              to: mockFromAddress, // same as from
              value: '0',
              data: [],
              nonce: '3',
              gasPrice: '',
              gasLimit: '',
              signOnly: false,
              signedTransaction: '',
            },
            chainId: '0x1',
            maxPriorityFeePerGas: '0x0',
            maxFeePerGas: '0x0',
            gasEstimation: undefined,
          },
        },
        txStatus: BraveWallet.TransactionStatus.Submitted,
        id: 'submitted-tx',
      }
      expect(isCancelTransaction(cancelTx, [submittedTx])).toBe(false)
    },
  )

  test('should return true for cancel transaction (legacy)', () => {
    const cancelTx = {
      ...mockEthSendTestTransaction,
      txDataUnion: {
        ethTxData1559: undefined,
        ethTxData: {
          ...mockEthTXData,
          value: '0',
          nonce: '2',
        },
      },
    }
    const submittedTx = {
      ...cancelTx,
      txStatus: BraveWallet.TransactionStatus.Submitted,
      id: 'submitted-tx',
    }
    expect(isCancelTransaction(cancelTx, [submittedTx])).toBe(true)
  })

  test('should return true for cancel transaction (EIP1559)', () => {
    const cancelTx = {
      ...mockEthSendTestTransaction,
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            to: mockFromAddress, // same as from
            value: '0',
            data: [],
            nonce: '4',
            gasPrice: '',
            gasLimit: '',
            signOnly: false,
            signedTransaction: '',
          },
          chainId: '0x1',
          maxPriorityFeePerGas: '0x0',
          maxFeePerGas: '0x0',
          gasEstimation: undefined,
        },
      },
    }
    const submittedTx = {
      ...cancelTx,
      txStatus: BraveWallet.TransactionStatus.Submitted,
      id: 'submitted-tx',
    }
    expect(isCancelTransaction(cancelTx, [submittedTx])).toBe(true)
  })

  test('should return true for cancel transaction with 0x0 value', () => {
    const cancelTx = {
      ...mockEthSendTestTransaction,
      txDataUnion: {
        ethTxData: {
          ...mockEthTXData,
          value: '0x0', // hex zero
        },
      },
    }
    const submittedTx = {
      ...cancelTx,
      txStatus: BraveWallet.TransactionStatus.Submitted,
      id: 'submitted-tx',
    }
    expect(isCancelTransaction(cancelTx, [submittedTx])).toBe(true)
  })

  test('should handle case insensitive address comparison', () => {
    const cancelTx = {
      ...mockEthSendTestTransaction,
      fromAccountId: {
        address: mockFromAddress.toLowerCase(),
      } as BraveWallet.AccountId,
      txDataUnion: {
        ethTxData: {
          ...mockEthTXData,
          to: mockFromAddress.toUpperCase(), // different case
          value: '0',
        },
      },
    }
    const submittedTx = {
      ...cancelTx,
      txStatus: BraveWallet.TransactionStatus.Submitted,
      id: 'submitted-tx',
    }
    expect(isCancelTransaction(cancelTx, [submittedTx])).toBe(true)
  })
})
