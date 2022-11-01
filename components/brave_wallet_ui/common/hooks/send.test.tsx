import { TextEncoder, TextDecoder } from 'util'
// @ts-expect-error
global.TextDecoder = TextDecoder
global.TextEncoder = TextEncoder

import * as React from 'react'
import { Provider } from 'react-redux'
import { renderHook, act } from '@testing-library/react-hooks'
import {
  configureStore,
  createListenerMiddleware,
  PayloadAction
} from '@reduxjs/toolkit'

import * as WalletActions from '../actions/wallet_actions'
import { BraveWallet } from '../../constants/types'

import { mockAccount } from '../constants/mocks'

import useSend from './send'
import { LibContext } from '../context/lib.context'

import * as MockedLib from '../async/__mocks__/lib'
import { createWalletReducer } from '../reducers/wallet_reducer'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { createSendCryptoReducer } from '../reducers/send_crypto_reducer'
import { mockBasicAttentionToken, mockBinanceCoinErc20Token, mockEthToken, mockMoonCatNFT, mockNewAssetOptions } from '../../stories/mock-data/mock-asset-options'

const mockSendAssetOptions = [
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockMoonCatNFT
]

const mockAccountWithAddress = {
  ...mockAccount,
  address: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f'
}

const makeStoreWithActionSpies = (actionSpies: Array<{
  actionType: string
  spy: typeof jest.fn
}>) => {
  const listener = createListenerMiddleware()

  const walletReducer = createWalletReducer({
    ...mockWalletState,
    accounts: [mockAccountWithAddress],
    selectedAccount: mockAccountWithAddress,
    fullTokenList: mockSendAssetOptions
  })

  // register spies to redux actions
  actionSpies.forEach(({ actionType, spy }) => {
    listener.startListening({
      type: actionType,
      effect: (action: PayloadAction) => {
        spy(action.payload as any)
      }
    })
  })

  const store = configureStore({
    reducer: {
      wallet: walletReducer,
      sendCrypto: createSendCryptoReducer({
        selectedSendAsset: mockSendAssetOptions[0],
        sendAmount: '',
        toAddress: '',
        toAddressOrUrl: '',
        showEnsOffchainLookupOptions: false,
        addressError: '',
        addressWarning: '',
        ensOffchainLookupOptions: undefined
      })
    },
    middleware: [listener.middleware]
  })

  return store
}

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
        <LibContext.Provider value={MockedLib as any}>
          {children}
        </LibContext.Provider>
      </Provider>
  }
}

describe('useSend hook', () => {
  it('Should find a ens address for bravey.eth and do a normal sendTransaction', async () => {
    let sendTransactionSpy = jest.fn()
    let sendERC20TransferSpy = jest.fn()
    let sendERC721TransferFromSpy = jest.fn()

    const store = makeStoreWithActionSpies([
      {
        actionType: WalletActions.sendTransaction.type,
        spy: sendTransactionSpy
      },
      {
        actionType: WalletActions.sendERC20Transfer.type,
        spy: sendERC20TransferSpy
      },
      {
        actionType: WalletActions.sendERC721TransferFrom.type,
        spy: sendERC721TransferFromSpy
      }
    ])

    const { result } = renderHook(
      () => useSend(),
      renderHookOptionsWithCustomStore(store)
    )

    // Sets values here
    await act(async () => {
      result.current.setToAddressOrUrl('bravey.eth')
      result.current.selectSendAsset(mockEthToken)
      result.current.setSendAmount('10')
    })

    // Expected return values
    expect(result.current.toAddress).toEqual('mockAddress3')
    expect(result.current.selectedSendAsset).toEqual(mockEthToken)
    expect(result.current.sendAmount).toEqual('10')
    expect(result.current.addressError).toEqual('')
    expect(result.current.addressWarning).toEqual('')

    // Submits transaction here
    await act(async () => result.current.submitSend())

    // Expected transaction calls here
    expect(sendTransactionSpy).toBeCalledWith(
      {
        from: mockAccountWithAddress.address,
        to: 'mockAddress3',
        value: '0x8ac7230489e80000',
        coin: BraveWallet.CoinType.ETH
      }
    )
    expect(sendERC20TransferSpy).toBeCalledTimes(0)
    expect(sendTransactionSpy).toBeCalledTimes(1)
    expect(sendERC721TransferFromSpy).toBeCalledTimes(0)
  })

  it('Should find a UD address for brave.crypto and do a sendERC20Transfer', async () => {
    let sendTransactionSpy = jest.fn()
    let sendERC20TransferSpy = jest.fn()
    let sendERC721TransferFromSpy = jest.fn()

    const store = makeStoreWithActionSpies([
      {
        actionType: WalletActions.sendTransaction.type,
        spy: sendTransactionSpy
      },
      {
        actionType: WalletActions.sendERC20Transfer.type,
        spy: sendERC20TransferSpy
      },
      {
        actionType: WalletActions.sendERC721TransferFrom.type,
        spy: sendERC721TransferFromSpy
      }
    ])

    const { result } = renderHook(
      () => useSend(),
      renderHookOptionsWithCustomStore(store)
    )

    // Sets values here
    await act(async () => {
      result.current.setToAddressOrUrl('brave.crypto')
      result.current.selectSendAsset(mockBasicAttentionToken)
      result.current.setSendAmount('300')
    })

    // Expected return values
    expect(result.current.toAddress).toEqual('mockAddress2')
    expect(result.current.selectedSendAsset).toEqual(mockBasicAttentionToken)
    expect(result.current.sendAmount).toEqual('300')
    expect(result.current.addressError).toEqual('')
    expect(result.current.addressWarning).toEqual('')

    // Submits transaction here
    act(() => result.current.submitSend())

    // Expected transaction calls here
    expect(sendERC20TransferSpy).toBeCalledWith(
      {
        contractAddress: mockBasicAttentionToken.contractAddress,
        from: mockAccountWithAddress.address,
        to: 'mockAddress2',
        value: '0x1043561a8829300000',
        coin: BraveWallet.CoinType.ETH
      }
    )
    expect(sendERC20TransferSpy).toBeCalledTimes(1)
    expect(sendTransactionSpy).toBeCalledTimes(0)
    expect(sendERC721TransferFromSpy).toBeCalledTimes(0)
  })

  it('Should find a ens address for brave.eth and do a sendERC721TransferFrom', async () => {
    let sendTransactionSpy = jest.fn()
    let sendERC20TransferSpy = jest.fn()
    let sendERC721TransferFromSpy = jest.fn()

    const store = makeStoreWithActionSpies([
      {
        actionType: WalletActions.sendTransaction.type,
        spy: sendTransactionSpy
      },
      {
        actionType: WalletActions.sendERC20Transfer.type,
        spy: sendERC20TransferSpy
      },
      {
        actionType: WalletActions.sendERC721TransferFrom.type,
        spy: sendERC721TransferFromSpy
      }
    ])

    const { result } = renderHook(
      () => useSend(),
      renderHookOptionsWithCustomStore(store)
    )

    // Sets values here
    await act(async () => {
      result.current.setToAddressOrUrl('brave.eth')
      result.current.selectSendAsset(mockNewAssetOptions[6])
    })

    // Expected return values
    expect(result.current.toAddress).toEqual('mockAddress2')
    expect(result.current.selectedSendAsset).toEqual(mockNewAssetOptions[6])
    expect(result.current.sendAmount).toEqual('1')
    expect(result.current.addressError).toEqual('')
    expect(result.current.addressWarning).toEqual('')

    // Submits transaction here
    act(() => result.current.submitSend())

    // Expected transaction calls here
    expect(sendERC721TransferFromSpy).toBeCalledWith(
      {
        contractAddress: mockNewAssetOptions[6].contractAddress,
        from: mockAccountWithAddress.address,
        to: 'mockAddress2',
        tokenId: '0x42a5',
        value: '',
        coin: BraveWallet.CoinType.ETH
      }
    )
    expect(sendERC20TransferSpy).toBeCalledTimes(0)
    expect(sendTransactionSpy).toBeCalledTimes(0)
    expect(sendERC721TransferFromSpy).toBeCalledTimes(1)
  })

  describe('check for not found ens and ud domains', () => {
    describe.each([
      ['ens', 'cool.eth'],
      ['ud', 'cool.crypto']
    ])('%s', (domainType, domainName) => {
      it(`Should not find a ${domainType} address for ${domainName} and return an address error`, async () => {
        let sendTransactionSpy = jest.fn()
        let sendERC20TransferSpy = jest.fn()
        let sendERC721TransferFromSpy = jest.fn()

        const store = makeStoreWithActionSpies([
          {
            actionType: WalletActions.sendTransaction.type,
            spy: sendTransactionSpy
          },
          {
            actionType: WalletActions.sendERC20Transfer.type,
            spy: sendERC20TransferSpy
          },
          {
            actionType: WalletActions.sendERC721TransferFrom.type,
            spy: sendERC721TransferFromSpy
          }
        ])

        const { result } = renderHook(
          () => useSend(),
          renderHookOptionsWithCustomStore(store)
        )

        // Sets values here
        await act(async () => {
          result.current.setToAddressOrUrl(domainName)
        })

        // Expected return values
        expect(result.current.toAddress).toEqual('')
        expect(result.current.addressError).toEqual('braveWalletNotDomain')
      })
    })
  })

  describe('check for address errors', () => {
    describe.each([
      [mockAccountWithAddress.address, 'braveWalletSameAddressError'],
      ['0x8b52c24d6e2600bdb8dbb6e8da849ed', 'braveWalletNotValidAddress'],
      ['0x0D8775F648430679A709E98d2b0Cb6250d2887EF', 'braveWalletContractAddressError']
    ])('%s', (toAddress, addressError) => {
      it(`Should return a ${addressError}`, async () => {
        let sendTransactionSpy = jest.fn()
        let sendERC20TransferSpy = jest.fn()
        let sendERC721TransferFromSpy = jest.fn()

        const store = makeStoreWithActionSpies([
          {
            actionType: WalletActions.sendTransaction.type,
            spy: sendTransactionSpy
          },
          {
            actionType: WalletActions.sendERC20Transfer.type,
            spy: sendERC20TransferSpy
          },
          {
            actionType: WalletActions.sendERC721TransferFrom.type,
            spy: sendERC721TransferFromSpy
          }
        ])

        const { result } = renderHook(
          () => useSend(),
          renderHookOptionsWithCustomStore(store)
        )

        // Sets values here
        await act(async () => {
          result.current.setToAddressOrUrl(toAddress)
        })

        // Expected return values
        expect(result.current.toAddress).toEqual(toAddress)
        expect(result.current.addressError).toEqual(addressError)
      })
    })
  })
})
