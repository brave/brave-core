import { renderHook } from '@testing-library/react-hooks'

import { mockAccount, mockERC20Token, mockNetwork } from '../constants/mocks'
import usePreset from './select-preset'

describe('usePreset hook', () => {
  it.each([
    ['25% of 1 ETH', '1000000000000000000', 0.25, '0.25'],
    ['50% of 1 ETH', '1000000000000000000', 0.50, '0.5'],
    ['75% of 1 ETH', '1000000000000000000', 0.75, '0.75'],
    ['100% of 1 ETH', '1000000000000000000', 1, '1'],
    ['100% of 1.000000000000000001 ETH', '1000000000000000001', 1, '1.000000000000000001'], // 1.000000000000000001 ---(100%)---> 1.000000000000000001
    ['100% of 50.297939 ETH', '50297939000000000000', 1, '50.297939'], // 50.297939 ---(100%)---> 50.297939
    ['25% of 0.0001 ETH', '100000000000000', 0.25, '0.000025'] // 0.0001 ---(25%)---> 0.000025
  ])('should compute %s correctly', (_, balance: string, percent, expected: string) => {
    const mockFunc = jest.fn()

    const { result: { current: calcPresetAmount } } = renderHook(() => usePreset(
      {
        ...mockAccount,
        tokenBalanceRegistry: {
          [mockERC20Token.contractAddress.toLowerCase()]: balance
        }
      },
      mockNetwork,
      mockERC20Token,
      mockERC20Token,
      mockFunc,
      jest.fn()
    ))

    calcPresetAmount('swap')(percent)
    expect(mockFunc.mock.calls.length).toBe(1)
    expect(mockFunc.mock.calls[0][0]).toBe(expected)
  })
})
