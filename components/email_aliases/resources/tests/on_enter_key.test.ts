// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { onEnterKeyForDiv, onEnterKeyForInput } from '../content/on_enter_key'
import { KeyboardEvent } from 'react'

describe('onEnterKey', () => {
  const testKeyPress = (key: string, shouldCall: boolean) => {
    let called = false
    const wrappedHandler = onEnterKeyForDiv(() => {
      called = true
    })
    wrappedHandler({ key } as KeyboardEvent<HTMLDivElement>)
    expect(called).toBe(shouldCall)
  }

  const testInputKeyPress = (key: string, shouldCall: boolean) => {
    let called = false
    const wrappedHandler = onEnterKeyForInput(() => {
      called = true
    })
    wrappedHandler({ innerEvent: { key } } as any)
    expect(called).toBe(shouldCall)
  }

  describe('onEnterKeyForDiv', () => {
    test('calls onSubmit when Enter key is pressed', () => {
      testKeyPress('Enter', true)
    })

    test('does not call onSubmit for other keys', () => {
      testKeyPress('Space', false)
    })
  })

  describe('onEnterKeyForInput', () => {
    test('calls onSubmit when Enter key is pressed', () => {
      testInputKeyPress('Enter', true)
    })

    test('does not call onSubmit for other keys', () => {
      testInputKeyPress('Space', false)
    })
  })
})
