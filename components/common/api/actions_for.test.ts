// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { actionsFor } from './actions_for'

describe('actionsFor', () => {
  // Simulates a class instance where methods are on the prototype
  // (like Mojo remotes)
  class MockRemote {
    private internalState = 'initial'

    methodA(arg: string) {
      this.internalState = arg
      return `A: ${arg}`
    }

    methodB(num: number) {
      return `B: ${num}, state: ${this.internalState}`
    }

    methodC() {
      return Promise.resolve('C result')
    }

    notExposed() {
      return 'should not be accessible'
    }
  }

  it('should extract specified methods from a class instance', () => {
    const remote = new MockRemote()
    const actions = actionsFor(remote, ['methodA', 'methodB'] as const)

    expect(actions.methodA).toBeDefined()
    expect(actions.methodB).toBeDefined()
    expect(typeof actions.methodA).toBe('function')
    expect(typeof actions.methodB).toBe('function')
  })

  it('should not include methods not in the keys array', () => {
    const remote = new MockRemote()
    const actions = actionsFor(remote, ['methodA'] as const)

    expect(actions.methodA).toBeDefined()
    // @ts-expect-error - methodB should not be on the result type
    expect(actions.methodB).toBeUndefined()
    // @ts-expect-error - notExposed should not be on the result type
    expect(actions.notExposed).toBeUndefined()
  })

  it('should bind methods to preserve `this` context', () => {
    const remote = new MockRemote()
    const actions = actionsFor(remote, ['methodA', 'methodB'] as const)

    // Call methodA which modifies internal state
    const resultA = actions.methodA('changed')
    expect(resultA).toBe('A: changed')

    // methodB should see the changed internal state
    const resultB = actions.methodB(42)
    expect(resultB).toBe('B: 42, state: changed')
  })

  it('should work with async methods', async () => {
    const remote = new MockRemote()
    const actions = actionsFor(remote, ['methodC'] as const)

    const result = await actions.methodC()
    expect(result).toBe('C result')
  })

  it('should return an object that can be spread', () => {
    const remote = new MockRemote()
    const actions = actionsFor(remote, ['methodA', 'methodB'] as const)

    // Verify the result is a plain object with own enumerable properties
    const keys = Object.keys(actions)
    expect(keys).toContain('methodA')
    expect(keys).toContain('methodB')
    expect(keys.length).toBe(2)

    // Should be spreadable
    const spread = { ...actions }
    expect(spread.methodA).toBeDefined()
    expect(spread.methodB).toBeDefined()
  })

  it('should work with an empty keys array', () => {
    const remote = new MockRemote()
    const actions = actionsFor(remote, [] as const)

    expect(Object.keys(actions)).toEqual([])
  })

  it('should handle objects that are not class instances', () => {
    const plainObject = {
      doSomething: (x: number) => x * 2,
      doSomethingElse: () => 'done',
    }

    const actions = actionsFor(plainObject, ['doSomething'] as const)

    expect(actions.doSomething(5)).toBe(10)
    // @ts-expect-error - doSomethingElse not in keys
    expect(actions.doSomethingElse).toBeUndefined()
  })

  it('should skip non-function properties', () => {
    const mixedObject = {
      method: () => 'result',
      property: 'string value',
      numberProp: 42,
    }

    // TypeScript would normally prevent this, but testing runtime behavior
    const actions = actionsFor(mixedObject, [
      'method',
      'property' as any,
    ] as const)

    expect(actions.method).toBeDefined()
    // Non-function properties should not be included
    expect((actions as any).property).toBeUndefined()
  })
})
