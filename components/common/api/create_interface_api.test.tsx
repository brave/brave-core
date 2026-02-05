// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { expect, jest, describe, it, beforeEach } from '@jest/globals'
import { act, renderHook } from '@testing-library/react'
import {
  createInterfaceApi,
  state,
  event,
  clearAllDataForTesting,
} from './create_interface_api'

describe('createInterfaceApi', () => {
  // Clear the shared QueryClient between tests to avoid cache pollution
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('should return results keyed by parameters', async () => {
    const api = createInterfaceApi({
      key: 'test',
      actions: {},
      endpoints: {
        getData: {
          query: (id: string) => {
            return Promise.resolve({ id })
          },
        },
      },
    })

    // No initial / placeholder data, so don't expect the hook
    // @ts-expect-error
    expect(api.useGetfDataData).toBeUndefined()

    // Typescript shouldn't allow an endpoint that isn't defined
    // @ts-expect-error
    expect(api.useDoesNotExist).toBeUndefined()
    // @ts-expect-error
    expect(api.doesNotExist).toBeUndefined()

    // Check fetch is keyed by parameter
    expect(await api.getData.fetch('1')).toEqual({ id: '1' })
    expect(await api.getData.fetch('2')).toEqual({ id: '2' })
    // Current should be keyed by parameter
    expect(api.getData.current('1')).toEqual({ id: '1' })
    expect(api.getData.current('2')).toEqual({ id: '2' })
    expect(api.getData.current('3')).toBeUndefined()
    // Update should be keyed by parameter
    api.getData.update('1', { id: '3' })
    expect(api.getData.current('1')).toEqual({ id: '3' })
    // reset should be keyed by parameter
    api.getData.reset('2')
    expect(api.getData.current('2')).toBeUndefined()
    expect(api.getData.current('1')).toEqual({ id: '3' })
    expect(api.getData.current('3')).toBeUndefined()
  })

  it('reset should return to placeholder data', async () => {
    const api = createInterfaceApi({
      key: 'test',
      actions: {},
      endpoints: {
        getData: {
          query: (id: string) => {
            return Promise.resolve({ id })
          },
          placeholderData: { id: 'initial' },
        },
      },
    })

    expect(api.getData.current('1')).toEqual({ id: 'initial' })
    expect(api.getData.current('2')).toEqual({ id: 'initial' })
    expect(api.getData.current('3')).toEqual({ id: 'initial' })
    await api.getData.fetch('1')
    await api.getData.fetch('3')
    expect(api.getData.current('1')).toEqual({ id: '1' })
    expect(api.getData.current('2')).toEqual({ id: 'initial' })
    expect(api.getData.current('3')).toEqual({ id: '3' })
    api.getData.reset('1')
    expect(api.getData.current('1')).toEqual({ id: 'initial' })
    expect(api.getData.current('2')).toEqual({ id: 'initial' })
    expect(api.getData.current('3')).toEqual({ id: '3' })
  })

  it('invalidate should cause a re-fetch', async () => {
    const mockedQuery = jest.fn((id: string) => {
      return Promise.resolve({ id })
    })
    const api = createInterfaceApi({
      key: 'test',
      actions: {},
      endpoints: {
        getData: {
          query: mockedQuery,
        },
      },
    })

    // Should not initially fetch
    expect(mockedQuery).not.toHaveBeenCalled()
    expect(api.getData.current('1')).toBeUndefined()
    await api.getData.fetch('1')
    expect(api.getData.current('1')).toEqual({ id: '1' })
    await api.getData.fetch('1')
    await api.getData.fetch('1')
    expect(mockedQuery).toHaveBeenCalledWith('1')
    expect(mockedQuery).toHaveBeenCalledTimes(1)

    // Invalidate triggers an immediate refetch (refetchType: 'all')
    api.getData.invalidate('1')
    // Wait for the async refetch to complete
    await api.getData.fetch('1')
    expect(mockedQuery).toHaveBeenCalledTimes(2)

    // Additional fetches don't cause more calls since data is fresh
    await api.getData.fetch('1')
    expect(mockedQuery).toHaveBeenCalledTimes(2)
  })

  it('invalidate should cause automatic re-fetch when useQuery hook is subscribed', async () => {
    let fetchCount = 0
    const mockedQuery = jest.fn((id: string) => {
      fetchCount++
      return Promise.resolve({ id, fetchCount })
    })
    const api = createInterfaceApi({
      key: 'test-invalidate',
      actions: {},
      endpoints: {
        getData: {
          query: mockedQuery,
        },
      },
    })

    function useTestQueryData() {
      const query = api.getData.useQuery('1')
      return query.data
    }

    // Render the hook which should trigger the initial fetch
    let hookResult = await act(async () => renderHook(useTestQueryData))

    // Initial fetch should have happened
    expect(mockedQuery).toHaveBeenCalledTimes(1)
    expect(mockedQuery).toHaveBeenCalledWith('1')

    await act(async () => hookResult.rerender())
    expect(hookResult.result.current).toEqual({ id: '1', fetchCount: 1 })

    // Invalidate the query while the hook is subscribed
    await act(async () => {
      api.getData.invalidate('1')
    })

    // Wait for the automatic re-fetch to complete
    await act(async () => hookResult.rerender())

    // Should have automatically re-fetched due to the active subscription
    expect(mockedQuery).toHaveBeenCalledTimes(2)
    expect(hookResult.result.current).toEqual({ id: '1', fetchCount: 2 })
  })

  it('supports state that is not queried', async () => {
    const api = createInterfaceApi({
      key: 'test',
      actions: {},
      endpoints: {
        myData: state({
          myValue: 'initial',
        }),
      },
    })

    expect(api.myData.current()).toEqual({ myValue: 'initial' })
    api.myData.update({ myValue: 'updated' })
    expect(api.myData.current()).toEqual({ myValue: 'updated' })
  })

  it('creates a hook that returns the query data', async () => {
    const mockedQuery = jest.fn((id: string) => {
      return Promise.resolve({ id })
    })
    const api = createInterfaceApi({
      key: 'test-hook-data',
      actions: {},
      endpoints: {
        getData: {
          query: mockedQuery,
        },
      },
    })

    // Should not initially fetch
    expect(mockedQuery).not.toHaveBeenCalled()
    expect(api.getData.current('1')).toBeUndefined()

    function useTestQueryData() {
      const query = api.getData.useQuery('1')
      return query.data
    }

    let hookResult = await act(async () => renderHook(useTestQueryData))

    // Having the hook should trigger the query immediately
    expect(mockedQuery).toHaveBeenCalledWith('1')
    expect(mockedQuery).toHaveBeenCalledTimes(1)
    expect(api.getData.current('1')).toEqual({ id: '1' })
    // And now the hook should have the data
    await act(async () => hookResult.rerender())
    expect(hookResult.result.current).toEqual({ id: '1' })
  })

  it('gets array data from a query', async () => {
    const api = createInterfaceApi({
      key: 'test-array-data',
      actions: {},
      endpoints: {
        getData: {
          query: (id: string) => {
            return Promise.resolve({ id })
          },
        },
        getList: {
          query: () => {
            return Promise.resolve([{ id: '1' }, { id: '2' }])
          },
        },
      },
    })

    function useTestQueryData() {
      const query = api.useGetList()
      return query.data
    }

    let hookResult = await act(async () => renderHook(useTestQueryData))

    expect(api.getList.current()).toEqual([{ id: '1' }, { id: '2' }])

    await act(async () => hookResult.rerender())
    expect(hookResult.result.current).toEqual([{ id: '1' }, { id: '2' }])
  })

  it('gets array data from a query with placeholder', async () => {
    const api = createInterfaceApi({
      key: 'test-array-placeholder',
      actions: {},
      endpoints: {
        getData: {
          query: (id: string) => {
            return Promise.resolve({ id })
          },
        },
        getList: {
          query: () => {
            return Promise.resolve([{ id: '1' }, { id: '2' }])
          },
          placeholderData: [] as { id: string }[],
        },
      },
    })

    // Generates the convenience function since we define placeholderData
    expect(api.useGetListData).not.toBeUndefined()

    function useTestQueryData() {
      return api.useGetList().getListData
    }

    let hookResult = await act(async () => renderHook(useTestQueryData))
    expect(hookResult.result.current).toEqual([])

    expect(api.getList.current()).toEqual([{ id: '1' }, { id: '2' }])

    await act(() => hookResult.rerender())
    expect(hookResult.result.current).toEqual([{ id: '1' }, { id: '2' }])
  })

  it('can create mutations', async () => {
    const mutationFn = jest.fn((isThing: boolean) => {
      return Promise.resolve({ isThing })
    })

    const globalOnMutateObserver = jest.fn()
    const hookOnMutateObserver = jest.fn()
    const callOnMutateObserver = jest.fn()

    function createMyApi() {
      const api = createInterfaceApi({
        key: 'test-mutations',
        endpoints: {
          doSomething: {
            mutation: (isThing: boolean): Promise<{ isThing: boolean }> =>
              mutationFn(isThing),
            onMutate(variables) {
              globalOnMutateObserver(variables)
            },
          },
        },
        actions: {},
      })
      return api
    }

    const api = createMyApi()

    expect(api.useDoSomething).toBeDefined()
    expect(typeof api.doSomething).toBe('function')

    // @ts-expect-error - non-mutation query properties current is not defined for mutations
    expect(api.doSomething.current).toBeUndefined()

    function useMutate() {
      const result = api.useDoSomething({
        // @ts-expect-error - verify defining a handler here does not work or prevent
        // global event handler from firing
        onSettled(result, error, variables) {
          hookOnMutateObserver(variables)
        },
      })
      return result
    }

    const hookResult = await renderHook(useMutate)

    // Should not be called when only rendered
    expect(mutationFn).not.toHaveBeenCalled()
    expect(globalOnMutateObserver).not.toHaveBeenCalled()

    // Perform mutation side-effect
    await act(async () =>
      hookResult.result.current.doSomething([true], {
        // Define a local event handler
        onSettled(result, error, input) {
          callOnMutateObserver(input, result)
        },
      }),
    )

    // Verify the hook gets the updated data when re-rendered. It might be available
    // earlier but it definitely should be on re-render given our "fetch" returns
    // immediately.
    await act(async () => hookResult.rerender())
    expect(hookResult.result.current.data).toEqual({ isThing: true })

    // Verify we used our mutation function with the expected args
    expect(mutationFn).toHaveBeenCalledTimes(1)
    expect(mutationFn).toHaveBeenCalledWith(true)

    // Verify the global and local event handlers
    expect(globalOnMutateObserver).toHaveBeenCalledWith([true])
    expect(callOnMutateObserver).toHaveBeenCalledWith([true], { isThing: true })
    expect(hookOnMutateObserver).not.toHaveBeenCalled()

    // Verify direct mutation (not a react hook)
    {
      globalOnMutateObserver.mockClear()
      callOnMutateObserver.mockClear()
      mutationFn.mockClear()
      const directResult = await api.doSomething([true], {
        onSettled(data, error, variables, context) {
          callOnMutateObserver(variables, data)
        },
      })
      expect(directResult).toEqual({ isThing: true })
      expect(globalOnMutateObserver).toHaveBeenCalledWith([true])
      expect(callOnMutateObserver).toHaveBeenCalledWith([true], {
        isThing: true,
      })
      expect(mutationFn).toHaveBeenCalledWith(true)
    }

    {
      globalOnMutateObserver.mockClear()
      callOnMutateObserver.mockClear()
      mutationFn.mockClear()
      const directResult = await api.doSomething([false], {
        onSettled(data, error, variables, context) {
          callOnMutateObserver(variables, data)
        },
      })
      expect(directResult).toEqual({ isThing: false })
      expect(globalOnMutateObserver).toHaveBeenCalledWith([false])
      expect(callOnMutateObserver).toHaveBeenCalledWith([false], {
        isThing: false,
      })
      expect(mutationFn).toHaveBeenCalledWith(false)
    }
  })

  it('can create void mutations with no parameters', async () => {
    const mutationFn = jest.fn(() => {
      return Promise.resolve()
    })

    function createMyApi() {
      const api = createInterfaceApi({
        key: 'test',
        endpoints: {
          doSomething: {
            mutation: () => {
              return mutationFn()
            },
          },
        },
        actions: {},
      })
      return api
    }

    const api = createMyApi()
    expect(api.doSomething).toBeDefined()
    expect(api.useDoSomething).toBeDefined()
    // @ts-expect-error - current is not defined for mutations
    expect(api.doSomething.current).toBeUndefined()
    expect(mutationFn).toHaveBeenCalledTimes(0)
    const result = await api.doSomething()
    expect(result).toBeUndefined()
    expect(mutationFn).toHaveBeenCalledTimes(1)
  })

  it('can create void mutations that have a parameter', async () => {
    const mutationFn = jest.fn((hi: string) => {
      return Promise.resolve()
    })

    function createMyApi() {
      const api = createInterfaceApi({
        key: 'test',
        endpoints: {
          doSomething: {
            mutation: (hi: string) => {
              return mutationFn(hi)
            },
          },
        },
        actions: {},
      })
      return api
    }

    const api = createMyApi()
    expect(api.doSomething).toBeDefined()
    expect(typeof api.doSomething).toBe('function')
    expect(api.doSomething).toBeDefined()
    // expect(api.useDoSomething).toBeUndefined()
    expect(mutationFn).toHaveBeenCalledTimes(0)
    const result = await api.doSomething(['4'])
    expect(result).toEqual(undefined)
    expect(mutationFn).toHaveBeenCalledTimes(1)
    expect(mutationFn).toHaveBeenCalledWith('4')
  })

  it('creates actions', () => {
    const doSomething = jest.fn((hi: string) => {})
    function createMyApi() {
      const api = createInterfaceApi({
        key: 'test',
        endpoints: {},
        actions: {
          doSomething(hi: string) {
            return doSomething(hi)
          },
          moreThings: {
            doAnotherThing() {},
          },
        },
      })
      return api
    }
    const api = createMyApi()

    expect(api.doSomething).toBeDefined()
    expect(api.moreThings.doAnotherThing).toBeDefined()
    expect(api.doSomething('4')).toBeUndefined()
    expect(doSomething).toHaveBeenCalledWith('4')
    expect(doSomething).toHaveBeenCalledTimes(1)
    expect(api.moreThings.doAnotherThing()).toBeUndefined()
  })

  it('updates the query cache when update is called', async () => {
    const api = createInterfaceApi({
      key: 'test',
      actions: {},
      endpoints: {
        getData: {
          query: (id: string) => {
            return Promise.resolve({ id })
          },
        },
        getList: {
          query: () => {
            return Promise.resolve([{ id: '1' }, { id: '2' }])
          },
        },
      },
    })

    // initially undefined
    expect(api.getList.current()).toBeUndefined()
    expect(api.getData.current('1')).toBeUndefined()

    // fetch
    await api.getList.fetch()
    await api.getData.fetch('1')
    // returned data matches
    expect(api.getList.current()).toEqual([{ id: '1' }, { id: '2' }])
    expect(api.getData.current('1')).toEqual({ id: '1' })

    // update object
    api.getData.update('1', { id: '3' })
    expect(api.getData.current('1')).toEqual({ id: '3' })

    // update array
    api.getList.update([{ id: '3' }, { id: '4' }])
    // check the array has not been spread to an object during the update()
    expect(api.getList.current()).toEqual([{ id: '3' }, { id: '4' }])
  })

  it('creates state queries with placeholder data', async () => {
    const api = createInterfaceApi({
      key: 'test-state-placeholder',
      actions: {},
      endpoints: {
        state: state({
          aProp: 'initial',
          bProp: true,
        }),
      },
    })

    // Ensure typescript knows, and actual state is, never undefined since we
    // provide initial data.
    expect(api.state.current().aProp).not.toBeUndefined()
    expect(api.state.current()).toEqual({ aProp: 'initial', bProp: true })

    // Render a hook and verify the query is not fetched (no error)
    function useStateData() {
      return api.state.useQuery()
    }

    let hookResult = await act(async () => renderHook(useStateData))

    expect(hookResult.result.current.data).toEqual({
      aProp: 'initial',
      bProp: true,
    })
    expect(hookResult.result.current.error).toBeNull()

    await act(async () => hookResult.rerender())

    expect(hookResult.result.current.data).toEqual({
      aProp: 'initial',
      bProp: true,
    })
    expect(hookResult.result.current.error).toBeNull()
  })

  it('creates state queries with no placeholder data', async () => {
    const api = createInterfaceApi({
      key: 'test-state-no-placeholder',
      actions: {},
      endpoints: {
        flag: state<boolean>(),
      },
    })

    // Ensure typescript knows, and actual state is, undefined when we don't
    // provide initial data.
    expect(api.flag.current()).toBeUndefined()
    // Ensure typescript knows, and actual state is, undefined when we don't
    // provide initial data.
    expect(api.flag.current()).toBeUndefined()

    // Render a hook and verify the query is not fetched (no error)
    function useStateData() {
      return api.flag.useQuery()
    }

    let hookResult = await act(async () => renderHook(useStateData))

    expect(hookResult.result.current.data).toBeUndefined()
    expect(hookResult.result.current.error).toBeNull()

    await act(async () => hookResult.rerender())

    expect(hookResult.result.current.data).toBeUndefined()
    expect(hookResult.result.current.error).toBeNull()

    // set data
    api.flag.update(true)

    await act(async () => hookResult.rerender())

    expect(hookResult.result.current.data).toBe(true)
    expect(hookResult.result.current.error).toBeNull()
    expect(api.flag.current()).toBe(true)
  })

  it('hook should update when api.update() is called with array placeholder data', async () => {
    // This test replicates the conversation history scenario:
    // - Query has placeholder data (empty array)
    // - Data is updated via update() (from Mojo events)
    // - Hook should reflect the updated data
    let fetchCount = 0
    const api = createInterfaceApi({
      key: 'test-update-array',
      actions: {},
      endpoints: {
        getHistory: {
          query: () => {
            fetchCount++
            return Promise.resolve([{ id: `${fetchCount}` }])
          },
          placeholderData: [] as { id: string }[],
        },
      },
    })

    function useHistoryData() {
      const query = api.useGetHistory()
      return {
        data: query.data,
        dataUpdatedAt: query.dataUpdatedAt,
        isPlaceholderData: query.isPlaceholderData,
      }
    }

    // Render the hook
    let hookResult = await act(async () => renderHook(useHistoryData))

    // Wait for prefetch to complete
    await act(async () => hookResult.rerender())

    // Should have fetched data
    expect(fetchCount).toBe(1)
    expect(hookResult.result.current.data).toEqual([{ id: '1' }])
    expect(hookResult.result.current.isPlaceholderData).toBe(false)

    // Now simulate a Mojo event updating the data via update()
    await act(async () => {
      api.getHistory.update((old) => [...old, { id: '2' }])
    })

    await act(async () => hookResult.rerender())

    // Hook should see the updated data
    expect(hookResult.result.current.data).toEqual([{ id: '1' }, { id: '2' }])
  })

  it('hook should update when api changes (simulating conversation switch)', async () => {
    // This test replicates the scenario where:
    // - API #1 is created with key 'conversation-1'
    // - Hook subscribes to API #1
    // - API #2 is created with key 'conversation-2'
    // - Hook should now use API #2's data via different query keys
    //
    // With shared QueryClient, unique keys ensure data isolation but intact
    // subscription to the correct instance.

    function createTestApi(key: string) {
      return createInterfaceApi({
        key: `conversation-${key}`,
        actions: {},
        endpoints: {
          getHistory: {
            query: () => Promise.resolve([{ id: key }]),
            placeholderData: [] as { id: string }[],
          },
        },
      })
    }

    let currentApi = createTestApi('1')

    function useHistoryData() {
      const query = currentApi.useGetHistory()
      return {
        data: query.data,
        key: (currentApi as any).__debugKey,
        status: query.status,
        fetchStatus: query.fetchStatus,
        isPlaceholderData: query.isPlaceholderData,
      }
    }

    // Use the shared APIQueryClientProvider wrapper
    // Render with API #1
    let hookResult = await act(async () => renderHook(useHistoryData))
    await act(async () => hookResult.rerender())

    expect(hookResult.result.current.data).toEqual([{ id: '1' }])
    expect(hookResult.result.current.key).toBe('conversation-1')

    // Switch to API #2 (simulating bindConversation)
    // This creates a new API with different key, so hooks use different query keys
    currentApi = createTestApi('2')

    // Re-render - the hook now uses the new API's query key
    await act(async () => hookResult.rerender())
    await act(async () => hookResult.rerender())
    await act(async () => hookResult.rerender())

    // With shared QueryClient and unique keys, hooks use the correct data
    expect(hookResult.result.current.key).toBe('conversation-2')
    expect(hookResult.result.current.data).toEqual([{ id: '2' }])

    // Now update API #2's data via update() (simulating Mojo event)
    await act(async () => {
      currentApi.getHistory.update([{ id: '2' }, { id: '3' }])
    })

    await act(async () => hookResult.rerender())
    await act(async () => hookResult.rerender())
    await act(async () => hookResult.rerender())

    expect(hookResult.result.current.data).toEqual([{ id: '2' }, { id: '3' }])
  })

  it('fires and handles events', () => {
    let emitter: (...args: [string, number]) => void = () => {}
    const api = createInterfaceApi({
      key: 'test-events',
      actions: {},
      endpoints: {},
      events: {
        myEvent: event<[], [string, number]>((emit) => {
          emitter = emit
        }),
      },
    })

    const observer = jest.fn()
    const unsubscribe = api.subscribeToMyEvent(observer)

    // manually call the event
    api.emitEvent('myEvent', ['4', 4])

    expect(observer).toHaveBeenCalled()
    expect(observer).toHaveBeenCalledWith('4', 4)

    // call the event from some outside function
    emitter('5', 5)
    expect(observer).toHaveBeenCalledTimes(2)
    expect(observer).toHaveBeenLastCalledWith('5', 5)

    // verify unsubscribe no longer gets notifications
    unsubscribe()
    api.emitEvent('myEvent', ['6', 6])
    expect(observer).toHaveBeenCalledTimes(2)
    emitter('5', 5)
    expect(observer).toHaveBeenCalledTimes(2)
  })
})
