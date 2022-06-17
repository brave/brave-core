import { renderHook, act } from '@testing-library/react-hooks'
import { useCopy } from './use-copy'

describe('useCopy Hook', () => {
  it('should have false as initial state', () => {
    const { result } = renderHook(() => useCopy())
    expect(result.current.copied).toBe(false)
  })

  it('should change copied to true when copyText is called', async () => {
    const { result } = renderHook(() => useCopy())
    await act(async () => {
      await result.current.copyText('some text')
    })
    expect(result.current.copied).toBe(true)
  })

  it('should change copied to false after timeout', async () => {
    const { result, waitForNextUpdate } = renderHook(() => useCopy())
    await act(async () => {
      await result.current.copyText('some text')
    })
    expect(result.current.copied).toBe(true)

    await waitForNextUpdate({
      timeout: 1500
    })
    expect(result.current.copied).toBe(false)
  })
})
