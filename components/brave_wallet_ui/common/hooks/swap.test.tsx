import * as React from 'react'
import { act, renderHook } from '@testing-library/react-hooks'

// Constants
import { mockAccount } from '../constants/mocks'
import { BraveWallet } from '../../constants/types'

// Hooks
import { TextEncoder, TextDecoder } from 'util'
global.TextDecoder = TextDecoder as any
global.TextEncoder = TextEncoder
import useSwap from './swap'

// Redux
import { Provider } from 'react-redux'
import { combineReducers, createStore } from 'redux'
import { createWalletReducer } from '../reducers/wallet_reducer'
import { createPageReducer } from '../../page/reducers/page_reducer'

// Mocks
import * as MockedLib from '../async/__mocks__/lib'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { mockPageState } from '../../stories/mock-data/mock-page-state'
import { LibContext } from '../context/lib.context'
import { mockBasicAttentionToken, mockEthToken } from '../../stories/mock-data/mock-asset-options'

const store = createStore(combineReducers({
  wallet: createWalletReducer(mockWalletState),
  page: createPageReducer(mockPageState)
}))

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

const renderHookOptions = renderHookOptionsWithCustomStore(store)

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

describe('useSwap hook', () => {
  beforeAll(() => {
    jest.useFakeTimers()
  })
  afterAll(() => {
    jest.clearAllTimers()
    jest.useRealTimers()
  })

  it('should initialize From and To assets', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useSwap(), renderHookOptions)

    act(() => {
      result.current.onSelectTransactAsset(mockEthToken, 'from')
      result.current.onSelectTransactAsset(mockBasicAttentionToken, 'to')
    })

    await waitForNextUpdate()

    expect(result.current.fromAsset).toEqual(mockEthToken)

    expect(result.current.toAsset).toEqual({
      coingeckoId: 'usd-coin',
      contractAddress: '0x07865c6e87b9f70255377e024ace6630c1eaa37f',
      decimals: 6,
      isErc20: true,
      isErc721: false,
      logo: 'chrome://erc-token-images/usdc.png',
      name: 'USD Coin',
      symbol: 'USDC',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID
    })
  })

  it('should return if network supports swap or not', async () => {
    const { result, waitFor } = renderHook(() => useSwap(), renderHookOptions)

    await waitFor(() => {
      expect(result.current.isSwapSupported).toBe(true)
    })
  })

  describe('token allowance', () => {
    let mockFn: jest.SpyInstance<Promise<string>, [contractAddress: string, ownerAddress: string, spenderAddress: string]>
    beforeEach(() => {
      mockFn = jest.spyOn(MockedLib, 'getERC20Allowance')
    })
    afterEach(() => {
      mockFn.mockRestore()
    })

    it('should not query allowance for native From asset', async () => {
      const { waitForNextUpdate } = renderHook(() => useSwap(), renderHookOptions)

      await waitForNextUpdate()

      expect(mockFn).not.toHaveBeenCalled()
    })

    it('should not query allowance if no quote', async () => {
      const { waitForNextUpdate, result, waitFor } = renderHook(() => useSwap(), renderHookOptions)

      // set from-asset to a non-native asset
      const USDC = result.current.swapAssetOptions[1]
      act(() => {
        result.current.setSwapQuote(undefined)
        result.current.onSelectTransactAsset(USDC, 'from')
      })

      await waitForNextUpdate()

      await waitFor(() => {
        expect(result.current.fromAsset?.contractAddress)
          .toBe(USDC.contractAddress)
      })

      expect(mockFn).not.toHaveBeenCalled()
    })

    it('should query allowance for an ERC20 From asset', async () => {
      const quote: BraveWallet.SwapResponse = {
        ...mockQuote,
        allowanceTarget: 'mockAllowanceTarget'
      }

      const { result } = renderHook(() => useSwap(), renderHookOptions)

      // set from-asset to a non-native asset
      const USDC = result.current.swapAssetOptions[1]
      await act(async () => {
        result.current.onSelectTransactAsset(USDC, 'from')
        result.current.setSwapQuote(quote)
      })

      expect(mockFn).toBeCalledWith(
        USDC.contractAddress,
        mockWalletState.accounts[0].address,
        quote.allowanceTarget
      )
    })
  })

  describe('swap validation errors', () => {
    it('should not return error if From and To amount are empty', async () => {
      // Step 1: Initialize the useSwap hook.
      const { result, waitForValueToChange, waitFor } = renderHook(() => useSwap(), renderHookOptions)

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      await act(async () => {
        result.current.setSwapQuote(mockQuote)
      })

      // OK: From and To amounts are 0, and swapValidationError is undefined.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.fromAmount).toBe('0')
        expect(result.current.toAmount).toBe('0')
        expect(result.current.swapValidationError).toBeUndefined()
      })
    })

    it('should return error if From amount has decimals overflow', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: ETH
      //    Balance:    1 ETH
      const { result, waitForValueToChange, waitFor } = renderHook(() => useSwap(), renderHookOptions)

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set From amount without decimal overflow and wait for at least
      // 1000ms to avoid debouncing.
      act(() => {
        result.current.onSwapInputChange('0.1', 'from')
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
        result.current.onSwapInputChange('0.1000000000000000000123', 'from')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'fromAmountDecimalsOverflow'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('fromAmountDecimalsOverflow')
      })
    })

    it('should return error if To amount has decimals overflow', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    To asset: BAT
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptions)

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set To amount and wait for at least 1000ms to avoid
      // debouncing.
      act(() => {
        result.current.onSwapInputChange('0.1', 'to')
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
        result.current.onSwapInputChange('0.1000000000000000000123', 'to')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'toAmountDecimalsOverflow'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('toAmountDecimalsOverflow')
      })
    })

    it('should return error if From ETH amount has insufficient balance', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: ETH
      //    Balance:    0.000000000000123456 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptions)

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount and wait for at least 1000ms to avoid
      // debouncing.
      act(() => {
        result.current.onSwapInputChange('0.000000000000123456', 'from')
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
        result.current.onSwapInputChange('1', 'from')
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
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptions)

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount and wait for at least 1000ms to avoid
      // debouncing.
      act(() => {
        result.current.onSwapInputChange('0.000000000000123456', 'from')
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
        result.current.onSwapInputChange('1', 'from')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientBalance'
      // KO: Test case times out
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientBalance')
      })
    })

    it('should return error if From ETH asset has insufficient balance for fees', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset: ETH
      //    Balance:    0.000000000000123456 ETH
      //    Quote fees: 0.000000000001000000 ETH

      const mockStore = createStore(combineReducers({
        wallet: createWalletReducer({
          ...mockWalletState,
          selectedAccount: {
            ...mockAccount,
            nativeBalanceRegistry: {
              [mockWalletState.selectedNetwork.chainId]: '123456' // 123456 Wei
            }
          }
        }),
        page: createPageReducer(mockPageState)
      }))

      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptionsWithCustomStore(mockStore))

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      await act(async () => {
        result.current.onSelectTransactAsset(mockEthToken, 'from') // From asset is ETH
        result.current.setSwapQuote({
          ...mockQuote,
          gasPrice: '10',
          gas: '100000'
        })
      })

      // OK: Assert for swapValidationError to be 'insufficientFundsForGas'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientFundsForGas')
      })
    })

    it('should return error if From ETH asset has insufficient balance for fees + swap', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  ETH
      //    From amount: 0.000000000000234560 ETH
      //    Quote fees:  0.000000000001000000 ETH
      //    Balance:     0.000000000001234560 ETH
      const mockStore = createStore(combineReducers({
        wallet: createWalletReducer({
          ...mockWalletState,
          selectedAccount: {
            ...mockAccount,
            nativeBalanceRegistry: {
              [mockWalletState.selectedNetwork.chainId]: '1234560' // 1234560 Wei
            }
          }
        }),
        page: createPageReducer(mockPageState)
      }))

      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptionsWithCustomStore(mockStore))

      act(() => {
        result.current.onSelectTransactAsset(mockEthToken, 'from')
        result.current.setSwapQuote({
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '234561' // 0.000000000000234561 ETH
        })
      })

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount, such that the value + fees is greater than
      // balance and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSwapInputChange('0.000000000000234561', 'from')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientFundsForGas'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientFundsForGas')
      })

      // OK: Assert for fromAmount to be '0.000000000000234561'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.fromAmount).toBe('0.000000000000234561')
      })
    })

    it('should return error if not enough allowance', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  USDC
      //    From amount: 10 USDC
      //    Quote fees:  0.000000000001 ETH
      //    Balance:     20 USDC

      const ETH = {
        ...mockWalletState.userVisibleTokensInfo[0],
        chainId: mockWalletState.selectedNetwork.chainId
      }
      const USDC = {
        ...mockWalletState.userVisibleTokensInfo[1],
        chainId: mockWalletState.selectedNetwork.chainId
      }

      const mockStore = createStore(combineReducers({
        wallet: createWalletReducer({
          ...mockWalletState,
          selectedAccount: {
            ...mockAccount,
            nativeBalanceRegistry: {
              [BraveWallet.MAINNET_CHAIN_ID]: '1000000000000000000', // 1 ETH
              [BraveWallet.ROPSTEN_CHAIN_ID]: '1000000000000000000' // 1 ETH
            },
            tokenBalanceRegistry: {
              [USDC.contractAddress.toLowerCase()]: '20000000000000000000' // 20 BAT
            }
          }
        }),
        page: createPageReducer(mockPageState)
      }))

      const { result, waitFor, waitForValueToChange } = renderHook(
        () => useSwap({
          fromAsset: USDC,
          toAsset: ETH
        }),
        renderHookOptionsWithCustomStore(mockStore)
      )

      // Step 2: Consume the update to isSwapSupported, so it does not fire
      // in the middle of a future update.
      await waitForValueToChange(() => result.current.isSwapSupported)

      // Step 3: Set a From amount, such that the value is greater than
      // token allowance, and wait for at least 1000ms to avoid debouncing.
      // set from-amount
      act(() => {
        result.current.onSwapInputChange('19', 'from') // From amount is 19 USDC
        jest.advanceTimersByTime(1001)
      })
      await waitFor(() => {
        expect(result.current.fromAmount).toBe('19')
      })

      // set-allowance
      act(() => {
        result.current.setAllowance('10')
        jest.advanceTimersByTime(1001)
      })
      await waitFor(() => {
        expect(result.current.allowance).toBe('10')
      })

      // OK: Assert for swapValidationError to be 'insufficientAllowance'.
      // KO: Test case times out.
      expect(result.current.swapValidationError).toBe('insufficientAllowance')
    })

    it('should return error if insufficient liquidity', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  ETH
      //    From amount: 0.1 ETH
      //    Quote fees:  0.000000000001 ETH
      //    Balance:     1 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptions)

      await waitForValueToChange(() => result.current.isSwapSupported)

      await act(async () => {
        result.current.setSwapQuote({
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '100000000000000000' // 0.1 ETH
        })
        result.current.setSwapError({
          code: 100, // SWAP_VALIDATION_ERROR_CODE
          reason: 'mockReason',
          validationErrors: [
            {
              field: 'mockField',
              code: 12345,
              reason: 'INSUFFICIENT_ASSET_LIQUIDITY'
            }
          ]
        })
      })

      // Step 2: Set a From amount, such that there is no validation error,
      // and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSwapInputChange('0.1', 'from')
        jest.advanceTimersByTime(1001)
      })

      // OK: Assert for swapValidationError to be 'insufficientLiquidity'.
      // KO: Test case times out.
      await waitFor(() => {
        expect(result.current.swapValidationError).toBe('insufficientLiquidity')
      })
    })

    it('should return error if gas estimation failed', async () => {
      // Step 1: Initialize the useSwap hook with the following parameters.
      //    From asset:  ETH
      //    From amount: 0.1 ETH
      //    Quote fees:  0.000000000001 ETH
      //    Balance:     1 ETH
      const { result, waitFor, waitForValueToChange } = renderHook(() => useSwap(), renderHookOptions)

      await waitForValueToChange(() => result.current.isSwapSupported)

      await act(async () => {
        result.current.setSwapQuote({
          ...mockQuote,
          gasPrice: '10',
          gas: '100000',
          sellAmount: '100000000000000000' // 0.1 ETH
        })
        result.current.setSwapError({
          code: 111, // gas estimation failed
          reason: 'Gas estimation failed'
        })
      })

      // Step 2: Set a From amount, such that there is no validation error,
      // and wait for at least 1000ms to avoid debouncing.
      act(() => {
        result.current.onSwapInputChange('0.1', 'from')
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
