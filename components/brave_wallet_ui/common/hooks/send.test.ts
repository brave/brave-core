import * as WalletActions from '../actions/wallet_actions'
import { renderHook, act } from '@testing-library/react-hooks'

import {
  GetEthAddrReturnInfo,
  GetChecksumEthAddressReturnInfo
} from '../../constants/types'

import { mockAccount } from '../constants/mocks'
import { AccountAssetOptions, NewAssetOptions } from '../../options/asset-options'

import useSend from './send'

const mockSendAssetOptions = [
  AccountAssetOptions[1],
  AccountAssetOptions[2],
  NewAssetOptions[6]
]

const mockENSValues = [
  {
    address: 'mockAddress2',
    name: 'brave.eth'
  },
  {
    address: 'mockAddress3',
    name: 'bravey.eth'
  }
]

const mockUDValues = [
  {
    address: 'mockAddress2',
    name: 'brave.crypto'
  },
  {
    address: 'mockAddress3',
    name: 'bravey.crypto'
  }
]

const mockAccountWithAddress = {
  ...mockAccount,
  address: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f'
}

const mockFindENSAddress = async (address: string) => {
  const foundAddress = mockENSValues.find((value) => value.name === address)
  if (foundAddress) {
    return { address: foundAddress.address, error: 0, errorMessage: '' } as GetEthAddrReturnInfo
  }
  return { address: '', error: 1, errorMessage: '' } as GetEthAddrReturnInfo
}

const mockFindUnstoppableDomainAddress = async (address: string) => {
  const foundAddress = mockUDValues.find((value) => value.name === address)
  if (foundAddress) {
    return { address: foundAddress.address, error: 0, errorMessage: '' } as GetEthAddrReturnInfo
  }
  return { address: '', error: 1, errorMessage: '' } as GetEthAddrReturnInfo
}

const mockGetChecksumEthAddress = async () => {
  return {} as GetChecksumEthAddressReturnInfo
}

describe('useSend hook', () => {
  let sendERC20TransferSpy: jest.SpyInstance
  let sendTransactionSpy: jest.SpyInstance
  let sendERC721TransferFromSpy: jest.SpyInstance

  sendERC20TransferSpy = jest.spyOn(WalletActions, 'sendERC20Transfer')
  sendTransactionSpy = jest.spyOn(WalletActions, 'sendTransaction')
  sendERC721TransferFromSpy = jest.spyOn(WalletActions, 'sendERC721TransferFrom')

  it('Should find a ens address for bravey.eth and do a normal sendTransaction', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useSend(
      mockFindENSAddress,
      mockFindUnstoppableDomainAddress,
      mockGetChecksumEthAddress,
      mockSendAssetOptions,
      mockAccount,
      WalletActions.sendERC20Transfer,
      WalletActions.sendTransaction,
      WalletActions.sendERC721TransferFrom,
      mockSendAssetOptions
    ))

    // Sets values here
    act(() => {
      result.current.onSetToAddressOrUrl('bravey.eth')
      result.current.onSelectSendAsset(AccountAssetOptions[0])
      result.current.onSetSendAmount('10')
    })
    await act(async () => {
      await waitForNextUpdate()
    })

    // Expected return values
    expect(result.current.toAddress).toEqual('mockAddress3')
    expect(result.current.selectedSendAsset).toEqual(AccountAssetOptions[0])
    expect(result.current.sendAmount).toEqual('10')
    expect(result.current.addressError).toEqual('')
    expect(result.current.addressWarning).toEqual('')

    // Submits transaction here
    act(() => result.current.onSubmitSend())

    // Expected transaction calls here
    expect(sendTransactionSpy).toBeCalledWith(
      {
        from: 'mockAddress',
        to: 'mockAddress3',
        value: '0x8ac7230489e80000'
      }
    )
    expect(sendERC20TransferSpy).toBeCalledTimes(0)
    expect(sendTransactionSpy).toBeCalledTimes(1)
    expect(sendERC721TransferFromSpy).toBeCalledTimes(0)
  })

  it('Should find a UD address for brave.crypto and do a sendERC20Transfer', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useSend(
      mockFindENSAddress,
      mockFindUnstoppableDomainAddress,
      mockGetChecksumEthAddress,
      mockSendAssetOptions,
      mockAccount,
      WalletActions.sendERC20Transfer,
      WalletActions.sendTransaction,
      WalletActions.sendERC721TransferFrom,
      mockSendAssetOptions
    ))

    // Sets values here
    act(() => {
      result.current.onSetToAddressOrUrl('brave.crypto')
      result.current.onSelectSendAsset(AccountAssetOptions[1])
      result.current.onSetSendAmount('300')
    })
    await act(async () => {
      await waitForNextUpdate()
    })

    // Expected return values
    expect(result.current.toAddress).toEqual('mockAddress2')
    expect(result.current.selectedSendAsset).toEqual(AccountAssetOptions[1])
    expect(result.current.sendAmount).toEqual('300')
    expect(result.current.addressError).toEqual('')
    expect(result.current.addressWarning).toEqual('')

    // Submits transaction here
    act(() => result.current.onSubmitSend())

    // Expected transaction calls here
    expect(sendERC20TransferSpy).toBeCalledWith(
      {
        contractAddress: AccountAssetOptions[1].contractAddress,
        from: 'mockAddress',
        to: 'mockAddress2',
        value: '0x1043561a8829300000'
      }
    )
    expect(sendERC20TransferSpy).toBeCalledTimes(1)
    expect(sendTransactionSpy).toBeCalledTimes(0)
    expect(sendERC721TransferFromSpy).toBeCalledTimes(0)
  })

  it('Should find a ens address for brave.eth and do a sendERC721TransferFrom', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useSend(
      mockFindENSAddress,
      mockFindUnstoppableDomainAddress,
      mockGetChecksumEthAddress,
      mockSendAssetOptions,
      mockAccount,
      WalletActions.sendERC20Transfer,
      WalletActions.sendTransaction,
      WalletActions.sendERC721TransferFrom,
      mockSendAssetOptions
    ))

    // Sets values here
    act(() => {
      result.current.onSetToAddressOrUrl('brave.eth')
      result.current.onSelectSendAsset(NewAssetOptions[6])
    })
    await act(async () => {
      await waitForNextUpdate()
    })

    // Expected return values
    expect(result.current.toAddress).toEqual('mockAddress2')
    expect(result.current.selectedSendAsset).toEqual(NewAssetOptions[6])
    expect(result.current.sendAmount).toEqual('1')
    expect(result.current.addressError).toEqual('')
    expect(result.current.addressWarning).toEqual('')

    // Submits transaction here
    act(() => result.current.onSubmitSend())

    // Expected transaction calls here
    expect(sendERC721TransferFromSpy).toBeCalledWith(
      {
        contractAddress: NewAssetOptions[6].contractAddress,
        from: 'mockAddress',
        to: 'mockAddress2',
        tokenId: '0x42a5',
        value: ''
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
        const { result, waitForNextUpdate } = renderHook(() => useSend(
          mockFindENSAddress,
          mockFindUnstoppableDomainAddress,
          mockGetChecksumEthAddress,
          mockSendAssetOptions,
          mockAccount,
          WalletActions.sendERC20Transfer,
          WalletActions.sendTransaction,
          WalletActions.sendERC721TransferFrom,
          mockSendAssetOptions
        ))

        // Sets values here
        act(() => {
          result.current.onSetToAddressOrUrl(domainName)
        })
        await act(async () => {
          await waitForNextUpdate()
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
      ['0x8b52c24d6e2600bdb8dbb6e8da849ed', 'braveWalletNotValidEthAddress'],
      ['0x0D8775F648430679A709E98d2b0Cb6250d2887EF', 'braveWalletContractAddressError']
    ])('%s', (toAddress, addressError) => {
      it(`Should return a ${addressError}`, async () => {
        const { result } = renderHook(() => useSend(
          mockFindENSAddress,
          mockFindUnstoppableDomainAddress,
          mockGetChecksumEthAddress,
          mockSendAssetOptions,
          mockAccountWithAddress,
          WalletActions.sendERC20Transfer,
          WalletActions.sendTransaction,
          WalletActions.sendERC721TransferFrom,
          mockSendAssetOptions
        ))

        // Sets values here
        act(() => {
          result.current.onSetToAddressOrUrl(toAddress)
        })

        // Expected return values
        expect(result.current.toAddress).toEqual(toAddress)
        expect(result.current.addressError).toEqual(addressError)
      })
    })
  })
})
