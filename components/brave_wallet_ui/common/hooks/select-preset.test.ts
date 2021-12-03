import { renderHook } from '@testing-library/react-hooks'

import { mockAccount, mockERC20Token } from '../constants/mocks'
import usePreset from './select-preset'

describe('usePreset hook', () => {
  it.each([
    ['25% of 1 ETH', '0xde0b6b3a7640000', 0.25, '0.25'],
    ['50% of 1 ETH', '0xde0b6b3a7640000', 0.50, '0.5'],
    ['75% of 1 ETH', '0xde0b6b3a7640000', 0.75, '0.75'],
    ['100% of 1 ETH', '0xde0b6b3a7640000', 1, '1'],
    ['100% of 1.000000000000000001 ETH', '0xde0b6b3a7640001', 1, '1.000000000000000001'], // 1.000000000000000001 ---(100%)---> 1.000000000000000001
    ['100% of 50.297939 ETH', '0x2ba062d07d5443000', 1, '50.297939'], // 50.297939 ---(100%)---> 50.297939
    ['25% of 0.0001 ETH', '0x5af3107a4000', 0.25, '0.000025'] // 0.0001 ---(25%)---> 0.000025
  ])('should compute %s correctly', (_, balance: string, percent, expected: string) => {
    const mockFunc = jest.fn()

    const { result: { current: calcPresetAmount } } = renderHook(() => usePreset(
      {
        ...mockAccount,
        tokens: [{
          asset: mockERC20Token,
          assetBalance: balance,
          fiatBalance: '0'
        }]
      },
      {
        asset: mockERC20Token,
        assetBalance: 'mockBalance',
        fiatBalance: 'mockBalance'
      },
      {
        asset: mockERC20Token,
        assetBalance: 'mockBalance',
        fiatBalance: 'mockBalance'
      },
      mockFunc,
      jest.fn()
    ))

    calcPresetAmount('swap')(percent)
    expect(mockFunc.mock.calls.length).toBe(1)
    expect(mockFunc.mock.calls[0][0]).toBe(expected)
  })
})
