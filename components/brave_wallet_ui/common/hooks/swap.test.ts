import { act, renderHook } from '@testing-library/react-hooks'

// Constants
import { mockAccount, mockNetwork, mockAssetPrices } from '../constants/mocks'
import { BraveWallet, WalletAccountType } from '../../constants/types'

// Options
import { AccountAssetOptions } from '../../options/asset-options'
const mockAccounts = [
  {
    ...mockAccount,
    balance: '1000000000000000000',
    tokenBalanceRegistry: {
      [AccountAssetOptions[1].contractAddress.toLowerCase()]: '1000000000000000000',
      [AccountAssetOptions[2].contractAddress.toLowerCase()]: '1000000000000000000'
    }
  } as WalletAccountType,
  {
    ...mockAccount,
    balance: '1000000000000000000',
    tokenBalanceRegistry: {
      [AccountAssetOptions[1].contractAddress.toLowerCase()]: '1000000000000000000',
      [AccountAssetOptions[2].contractAddress.toLowerCase()]: '1000000000000000000'
    }
  } as WalletAccountType
]

const userVisibleTokensInfo = [AccountAssetOptions[1], AccountAssetOptions[2]] // BNB, BTC
const getBuyAssets = async () => {
  return await userVisibleTokensInfo
}

const mockVisibleList = [
  AccountAssetOptions[0],
  AccountAssetOptions[1]
]

// Actions
import * as WalletPageActions from '../../page/actions/wallet_page_actions'
import * as WalletActions from '../actions/wallet_actions'

// Hooks
import useSwap from './swap'
import * as useAssets from './assets'

jest.useFakeTimers()

async function mockGetERC20Allowance (contractAddress: string, ownerAddress: string, spenderAddress: string) {
  return '1000000000000000000' // 1 unit
}

const mockIsSwapSupportedFactory = (expected: boolean) =>
  async (network: BraveWallet.EthereumChain) =>
    expected

const mockQuote = {
  allowanceTarget: '',
  price: '',
  guaranteedPrice: '',
  to: '',
  data: '',
  value: '',
  gas: '0',
  estimatedGas: '0',
  gasPrice: '0',
  protocolFee: '0',
  minimumProtocolFee: '0',
  buyTokenAddress: '',
  sellTokenAddress: '',
  buyAmount: '0',
  sellAmount: '0',
  sellTokenToEthRate: '1',
  buyTokenToEthRate: '1'
} as BraveWallet.SwapResponse

beforeEach(() => {
  // Clear all mocks
  jest.clearAllMocks()
})

describe('useSwap hook', () => {
  it('should initialize From and To assets', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useSwap(
      mockAccounts,
      AccountAssetOptions,
      mockVisibleList,
      mockAssetPrices,
      getBuyAssets,
      mockAccount,
      mockNetwork,
      WalletPageActions.fetchPageSwapQuote,
      mockGetERC20Allowance,
      WalletActions.approveERC20Allowance,
      mockIsSwapSupportedFactory(true)
    ))

    await waitForNextUpdate()

    expect(result.current.fromAsset).toBe(AccountAssetOptions[0])
    expect(result.current.toAsset).toBe(AccountAssetOptions[1])
  })

  it('should return if network supports swap or not', async () => {
    const { result, waitFor } = renderHook(() => useSwap(
      mockAccounts,
      AccountAssetOptions,
      mockVisibleList,
      mockAssetPrices,
      getBuyAssets,
      mockAccount,
      mockNetwork,
      WalletPageActions.fetchPageSwapQuote,
      mockGetERC20Allowance,
      WalletActions.approveERC20Allowance,
      mockIsSwapSupportedFactory(true)
    ))

    await waitFor(() => {
      expect(result.current.isSwapSupported).toBe(true)
    })
  })

  describe('token allowance', () => {
    it('should not query allowance for native From asset', async () => {
      const mockFn = jest.fn(() => Promise.resolve('mockErc20Allowance'))

      const { waitForNextUpdate } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        mockVisibleList,
        mockAssetPrices,
        getBuyAssets,
        mockAccount,
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockFn,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        mockQuote
      ))

      await waitForNextUpdate()

      expect(mockFn).not.toHaveBeenCalled()
    })

    it('should not query allowance if no quote', async () => {
      const mockFn = jest.fn(() => Promise.resolve('mockErc20Allowance'))

      const { waitForNextUpdate } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        mockVisibleList,
        mockAssetPrices,
        getBuyAssets,
        mockAccount,
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockFn,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true)
      ))

      await waitForNextUpdate()

      expect(mockFn).not.toHaveBeenCalled()
    })

    it('should query allowance for an ERC20 From asset', async () => {
      const quote: BraveWallet.SwapResponse = {
        ...mockQuote,
        allowanceTarget: 'mockAllowanceTarget'
      }

      const mockFn = jest.fn(() => Promise.resolve('mockErc20Allowance'))

      const { waitForNextUpdate } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        userVisibleTokensInfo,
        mockAssetPrices,
        getBuyAssets,
        mockAccounts[0],
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockFn,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        quote
      ))

      await waitForNextUpdate()

      expect(mockFn).toBeCalledWith(
        userVisibleTokensInfo[0].contractAddress,
        mockAccount.address,
        quote.allowanceTarget
      )
    })
  })

  describe('swap validation errors', () => {
    it('should not return error if From and To amount are empty', async () => {
      // Step 1: Initialize the useSwap hook.
      const { result, waitForValueToChange, waitFor } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        userVisibleTokensInfo,
        mockAssetPrices,
        getBuyAssets,
        mockAccounts[0],
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        mockQuote
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // OK: From and To amounts are 0, and swapValidationError is undefined.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.fromAmount).toBe('0')
        expect(result.current.toAmount).toBe('0')
        expect(result.current.swapValidationError).toBeUndefined()
      })
    })

    it('should return error if From amount has decimals overflow', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: userVisibleTokensInfo, // From Asset: BAT
        swapToAssetOptions: userVisibleTokensInfo,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: BAT
      //    Balance:    1 BAT
      const { result, waitForValueToChange, waitFor } = renderHook(() => useSwap(
        mockAccounts,
        userVisibleTokensInfo,
        userVisibleTokensInfo,
        mockAssetPrices,
        getBuyAssets,
        {
          ...mockAccount,
          balance: '1000000000000000000' // 1 ETH
        },
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        mockQuote
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set From amount without decimal overflow and wait for at least
      // 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('0.1')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be undefined.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBeUndefined()
      })

      // Step 4: Set From amount with decimal overflow and wait for at least
      // 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('0.1000000000000000000123')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'fromAmountDecimalsOverflow'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('fromAmountDecimalsOverflow')
      })
    })

    it('should return error if To amount has decimals overflow', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: AccountAssetOptions,
        swapToAssetOptions: mockVisibleList, // From Asset: BAT
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    To asset: BAT
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        mockVisibleList,
        mockAssetPrices,
        getBuyAssets,
        mockAccount,
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        mockQuote
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set To amount and wait for at least 1000ms to avoid
      // debouncing.
      act(() => {
        result.current.onSetToAmount('0.1')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be undefined.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBeUndefined()
      })

      // Step 4: Set To amount with a decimal overflow and wait for at least
      // 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetToAmount('0.1000000000000000000123')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'toAmountDecimalsOverflow'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('toAmountDecimalsOverflow')
      })
    })

    it('should return error if From ETH amount has insufficient balance', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: AccountAssetOptions, // From Asset: ETH
        swapToAssetOptions: mockVisibleList,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: ETH
      //    Balance:    0.000000000000123456 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        mockVisibleList,
        mockAssetPrices,
        getBuyAssets,
        mockAccount, // Balance: 123456 Wei
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        mockQuote
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount and wait for at least 1000ms to avoid
      // debouncing.
      act(() => {
        result.current.onSetFromAmount('0.000000000000123456')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be undefined.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBeUndefined()
      })

      // Step 4: Set a From amount greater than balance and wait for at least
      // 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('1')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientBalance'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientBalance')
      })
    })

    it('should return error if From token amount has insufficient balance', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: BAT
      //    Balance:    0.000000000000123456 ETH

      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: userVisibleTokensInfo, // From Asset: BAT
        swapToAssetOptions: userVisibleTokensInfo,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })

      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        userVisibleTokensInfo,
        userVisibleTokensInfo,
        mockAssetPrices,
        getBuyAssets,
        {
          ...mockAccount,
          tokenBalanceRegistry: {
            [AccountAssetOptions[1].contractAddress.toLowerCase()]: '123456' // 0.000000000000123456 BAT
          }
        },
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        mockQuote
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount and wait for at least 1000ms to avoid
      // debouncing.
      act(() => {
        result.current.onSetFromAmount('0.000000000000123456')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be undefined.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBeUndefined()
      })

      // Step 4: Set a From amount greater than balance and wait for at least
      // 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('1')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientBalance'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientBalance')
      })
    })

    it('should return error if From ETH asset has insufficient balance for fees', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: AccountAssetOptions, // From Asset: ETH
        swapToAssetOptions: AccountAssetOptions,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: ETH
      //    Balance:    0.000000000000123456 ETH
      //    Quote fees: 0.000000000001000000 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        mockVisibleList,
        mockAssetPrices,
        getBuyAssets,
        mockAccount, // Balance: 123456 Wei
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        {
          ...mockQuote,
          gasPrice: '10',
          gas: '100000'
        }
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // OK: Assert for swapValidationError to be 'insufficientEthBalance'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientEthBalance')
      })
    })

    it('should return error if From ETH asset has insufficient balance for fees + swap', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: AccountAssetOptions, // From Asset: ETH
        swapToAssetOptions: AccountAssetOptions,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })

      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  ETH
      //    From amount: 0.000000000000234560 ETH
      //    Quote fees:  0.000000000001000000 ETH
      //    Balance:     0.000000000001234560 ETH
      const mockAccounts = [
        {
          ...mockAccount,
          balance: '1234560'
        } as WalletAccountType,
        {
          ...mockAccount,
          balance: '1234560'
        } as WalletAccountType
      ]

      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        mockVisibleList,
        mockAssetPrices,
        getBuyAssets,
        mockAccounts[0], // Balance: 123456 Wei
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        {
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '234561' // 0.000000000000234561 ETH
        }
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount, such that the value + fees is greater than
      // balance and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('0.000000000000234561')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientEthBalance'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientEthBalance')
      })

      // OK: Assert for fromAmount to be '0.000000000000234561'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.fromAmount).toBe('0.000000000000234561')
      })
    })

    it('should return error if not enough allowance', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: userVisibleTokensInfo, // From Asset: BAT
        swapToAssetOptions: userVisibleTokensInfo,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  BAT
      //    From amount: 10 BAT
      //    Quote fees:  0.000000000001 ETH
      //    Balance:     20 BAT
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        userVisibleTokensInfo,
        userVisibleTokensInfo,
        mockAssetPrices,
        getBuyAssets,
        {
          ...mockAccount,
          balance: '1000000000000000000', // 1 ETH
          tokenBalanceRegistry: {
            [AccountAssetOptions[1].contractAddress.toLowerCase()]: '20000000000000000000' // 20 BAT
          }
        },
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        {
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '10000000000000000000' // 10 BAT
        }
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount, such that the value is greater than
      // token allowance, and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('10')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientAllowance'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientAllowance')
      })
    })

    // TODO: Fix failing test case.
    it('should return error if insufficient liquidity', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: AccountAssetOptions,
        swapToAssetOptions: AccountAssetOptions,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  ETH
      //    From amount: 0.1 ETH
      //    Quote fees:  0.000000000001 ETH
      //    Balance:     1 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        AccountAssetOptions,
        mockAssetPrices,
        getBuyAssets,
        {
          ...mockAccount,
          balance: '1000000000000000000' // 1 ETH
        },
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        {
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '100000000000000000' // 0.1 ETH
        },
        {
          code: 100, // SWAP_VALIDATION_ERROR_CODE
          reason: 'mockReason',
          validationErrors: [
            {
              field: 'mockField',
              code: 12345,
              reason: 'INSUFFICIENT_ASSET_LIQUIDITY'
            }
          ]
        }
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount, such that there is no validation error,
      // and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('0.1')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientLiquidity'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientLiquidity')
      })
    })

    it('should return error if gas estimation failed', async () => {
      const useAssetsMock = jest.spyOn(useAssets, 'default')
      useAssetsMock.mockReturnValue({
        swapFromAssetOptions: AccountAssetOptions, // From Asset: ETH
        swapToAssetOptions: AccountAssetOptions,
        sendAssetOptions: [],
        buyAssetOptions: [],
        panelUserAssetList: []
      })

      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  ETH
      //    From amount: 0.1 ETH
      //    Quote fees:  0.000000000001 ETH
      //    Balance:     1 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(
        mockAccounts,
        AccountAssetOptions,
        AccountAssetOptions,
        mockAssetPrices,
        getBuyAssets,
        {
          ...mockAccount,
          balance: '1000000000000000000' // 1 ETH
        },
        mockNetwork,
        WalletPageActions.fetchPageSwapQuote,
        mockGetERC20Allowance,
        WalletActions.approveERC20Allowance,
        mockIsSwapSupportedFactory(true),
        {
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '100000000000000000' // 0.1 ETH
        },
        {
          code: 111, // gas estimation failed
          reason: 'Gas estimation failed'
        }
      ))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount, such that there is no validation error,
      // and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSetFromAmount('0.1')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'unknownError'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('unknownError')
      })
    })
  })
})
