// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { eventsFor, partialReceiver } from './events_for'
import { createInterfaceApi, EventDef } from './create_interface_api'

describe('eventsFor', () => {
  class TestInterface {
    testMethod(arg1: string, arg2: number) {}
    anotherMethod(arg1: boolean) {}
  }

  class TestReceiver {
    public handler: TestInterface
    constructor(handler: TestInterface) {
      this.handler = handler
    }
  }

  it('partialReceiver should create a receiver with the provided handler', () => {
    const partialInterface = {
      testMethod: (arg1: string, arg2: number) => {
      },
    }

    const builtReceiver = partialReceiver(
      TestInterface,
      TestReceiver,
      partialInterface,
    )
    // should get these arguments passed through to partialInterface
    builtReceiver.handler.testMethod('test', 42)
    // shouldn't fail
    builtReceiver.handler.anotherMethod(true)
  })

  it('should create an interface with the provided methods', () => {
    const testMethodObserver = jest.fn()
    let observer: TestInterface | undefined
    const e1 = eventsFor(
      TestInterface,
      {
        testMethod: (arg1, arg2) => {
          testMethodObserver(arg1, arg2)
        },
      },
      (instance) => {
        expect(instance.testMethod).toBeDefined()
        expect(instance.anotherMethod).toBeDefined()
        observer = instance
      },
    )

    const emitter = jest.fn()
    e1.testMethod.registerEmitter(emitter)

    expect(e1.testMethod).toBeDefined()
    // @ts-expect-error verify typescript catches invalid arguments
    expect(e1.anotherMethod).not.toBeDefined()

    // Check that typescript can infer the event arguments and they match
    // the interface method arguments.
    type TestMethodArgs = Parameters<TestInterface['testMethod']>
    const testMethodArgs: TestMethodArgs = ['test', 42]
    expect(testMethodArgs).toEqual(['test', 42])
    type GeneratedMethodArgs = typeof e1.testMethod extends EventDef<[], infer Args> ? Args : never
    const testMethodGeneratedArgs: GeneratedMethodArgs = testMethodArgs
    expect(testMethodGeneratedArgs).toEqual(['test', 42])

    // calling the event calls the emitter
    expect(observer).not.toBeUndefined()
    observer!.testMethod('test', 42)
    expect(testMethodObserver).toHaveBeenCalledWith('test', 42)
    expect(emitter).toHaveBeenCalledWith('test', 42)
  })

  it('fires and handles events with createInterfaceApi', () => {
    let eventHandler: TestInterface | undefined
    const api = createInterfaceApi({
      actions: {},
      endpoints: {},
      events: eventsFor(
        TestInterface,
        {
          testMethod: (arg1, arg2) => {}
        },
        (instance) => {
          eventHandler = instance
        }
      )
    })

    expect(eventHandler).not.toBeUndefined()
    // If we fire the observer method, it should call the event handler
    const observer = jest.fn()
    const unsubscribe = api.subscribeToTestMethod(observer)
    eventHandler?.testMethod('test', 42)
    expect(observer).toHaveBeenCalledWith('test', 42)
    unsubscribe()
    eventHandler?.testMethod('test', 43)
    expect(observer).not.toHaveBeenCalledWith('test', 43)
    expect(observer).toHaveBeenCalledTimes(1)
  })

})