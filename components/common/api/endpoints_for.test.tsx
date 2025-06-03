// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, renderHook } from '@testing-library/react'
import { endpointsFor } from './endpoints_for'
import { createInterfaceApi } from './create_interface_api'

describe('endpointsFor', () => {
  class Test {
    // Test we don't ruin `this` in the queries
    public localState = 'local'

    getState() {
      this.localState = 'changed'
      return Promise.resolve({
        state: {
          aProp: 'fetched',
          bProp: false,
        },
      })
    }

    getSimple(id: string) {
      return Promise.resolve({
        simple: 'ad',
      })
    }

    getMultiple(): Promise<{ multiple: { a: string }[] }> {
      return new Promise(resolve => window.setTimeout(() => resolve({
        multiple: [{ a: 'a' }, { a: 'b' }, { a: 'c' }],
      }), 100))
    }

    getScreenshots: () => Promise<{ screenshots: string[] }>

    dontWant: () => Promise<{ something: string }>
  }

  const test: Test = new Test()

  const testResult = endpointsFor(test, {
    getState: {
      response: (result) => result.state,
      prefetchWithArgs: [],
      initialData: {
        aProp: 'initial',
        bProp: true,
      },
    },
    getSimple: {
      response: (result) => result.simple,
      prefetchWithArgs: ['4'],
      initialData: 'ad',
    },
    getMultiple: {
      response: (result) => result.multiple,
      prefetchWithArgs: [],
    },
    getScreenshots: {
      mutationResponse: (result: { screenshots: string[] }) => result.screenshots,
    },
  })

  it('should create endpoints with the correct structure', () => {
    // getScreenshots should generate a mutation
    expect(testResult.getScreenshots.mutation).toBeDefined()

    // getState should generate a query
    expect(testResult.getState.query).toBeDefined()
    // await getState.query() and check the result
    expect(testResult.getState.query()).resolves.toEqual({
      aProp: 'fetched',
      bProp: false,
    })

    // dontWant should not be defined
    // @ts-expect-error
    expect(testResult.dontWant).toBeUndefined()
  })

  it('should create an interface API with the endpoints', () => {
    // Can the combination of createInterfaceAPI and endpointsFor
    // infer the return type of the query, including initialData?
    const api = createInterfaceApi({
      actions: {},
      endpoints: {
        ...testResult,
      },
    })

    expect(api.getState.current).not.toBeUndefined()
  })

  it('should keep arrays as arrays when combined with createInterfaceApi', async () => {
    const api = createInterfaceApi({
      actions: {},
      endpoints: {
        ...endpointsFor(test, {
          getMultiple: {
            response: (a) => {
              return a.multiple
            },
          }
        })
      }
    })

    function useTestQueryData() {
      const query = api.useGetMultiple()
      return query.getMultipleData
    }
    let hookResult = await act(() => renderHook(useTestQueryData))
    expect(hookResult.result.current).toEqual(undefined)

    await act(() => new Promise(resolve => setTimeout(resolve, 200))) // wait for the promise to resolve

    expect(api.getMultiple.current()).toEqual([{ a: 'a' }, { a: 'b' }, { a: 'c' }])

    await act(() => hookResult.rerender())
    expect(hookResult.result.current).toEqual([{ a: 'a' }, { a: 'b' }, { a: 'c' }])
  })
})