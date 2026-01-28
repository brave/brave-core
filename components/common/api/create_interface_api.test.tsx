// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { expect, jest, test, describe, it } from '@jest/globals'
import { act, renderHook } from '@testing-library/react'
import * as React from 'react'
import { createInterfaceApi, state, event } from './create_interface_api'

describe('createInterfaceApi', () => {
  it('should return results keyed by parameters', async () => {
    const api = createInterfaceApi({
      actions: {},
      endpoints: {
        getData: {
          query: (id: string) => {
            return Promise.resolve({ id })
          },
        },
      },
    })

    // Check fetch is keyed by parameter
    expect(await api.endpoints.getData.fetch('1')).toEqual({ id: '1' })
    expect(await api.endpoints.getData.fetch('2')).toEqual({ id: '2' })
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
    // Now invalidate the query
    api.getData.invalidate('1')
    // Shouldn't re-fetch until asked to
    expect(mockedQuery).toHaveBeenCalledTimes(1)
    await api.getData.fetch('1')
    await api.getData.fetch('1')
    expect(mockedQuery).toHaveBeenCalledTimes(2)
  })

  it('supports state that is not queried', async () => {
    const api = createInterfaceApi({
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
      const query = api.endpoints.getData.useQuery('1')
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

    function useTestQueryData() {
      const query = api.useGetList()
      return query.data
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
        // @ts-ignore - verify defining a handler here does not work or prevent
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
    expect(api.endpoints.doSomething.mutate).toBeDefined()
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
    expect(api.actions.doSomething).toBeDefined()
    expect(api.actions.moreThings.doAnotherThing).toBeDefined()
    expect(api.actions.doSomething('4')).toBeUndefined()
    expect(doSomething).toHaveBeenCalledWith('4')
    expect(doSomething).toHaveBeenCalledTimes(1)
    expect(api.actions.moreThings.doAnotherThing()).toBeUndefined()
  })

  it('updates the query cache when update is called', async () => {
    const api = createInterfaceApi({
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

  it('creates state queries with no query fetching', async () => {
    const queryFn = jest.fn(() =>
      Promise.resolve({ aProp: 'fetched', bProp: false }),
    )
    const api = createInterfaceApi({
      actions: {},
      endpoints: {
        state: state({
          aProp: 'initial',
          bProp: true,
        }),
      },
    })

    // Should not fetch the query
    expect(queryFn).not.toHaveBeenCalled()
    expect(api.state.current()).toEqual({ aProp: 'initial', bProp: true })
    expect(queryFn).not.toHaveBeenCalled()

    // Render a hook and verify the query is not fetched
    function useStateData() {
      const stateData = api.state.useQuery()
      return stateData.data
    }
    let hookResult = await act(async () => renderHook(useStateData))
    expect(hookResult.result.current).toEqual({ aProp: 'initial', bProp: true })
    expect(queryFn).not.toHaveBeenCalled()
    await act(async () => hookResult.rerender())
    expect(hookResult.result.current).toEqual({ aProp: 'initial', bProp: true })
    expect(queryFn).not.toHaveBeenCalled()
  })

  it('fires and handles events', () => {
    let emitter: (...args: [String, number]) => void = () => {}
    const api = createInterfaceApi({
      actions: {},
      endpoints: {},
      events: {
        myEvent: event<[], [String, number]>((emit) => {
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
