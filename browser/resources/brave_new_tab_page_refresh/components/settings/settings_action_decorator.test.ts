/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { decorateActionsWithMetrics } from './settings_action_decorator'
import { NewTabActions, defaultNewTabActions } from '../../state/new_tab_state'

describe('decorateActionsWithMetrics', () => {
  let mockReportEdited: jest.Mock
  let newTabActions: NewTabActions

  beforeEach(() => {
    mockReportEdited = jest.fn()
    newTabActions = {
      ...defaultNewTabActions(),
      reportCustomizeDialogEdited: mockReportEdited,
    }
  })

  test('calls reportCustomizeDialogEdited before calling the action', () => {
    const mockAction = jest.fn()
    const actions = { doSomething: mockAction }

    const decorated = decorateActionsWithMetrics(newTabActions, actions)
    decorated.doSomething()

    expect(mockReportEdited).toHaveBeenCalledTimes(1)
    expect(mockAction).toHaveBeenCalledTimes(1)
  })

  test('passes arguments to the wrapped action', () => {
    const mockAction = jest.fn()
    const actions = { doSomething: mockAction }

    const decorated = decorateActionsWithMetrics(newTabActions, actions)
    decorated.doSomething('arg1', 'arg2', 123)

    expect(mockAction).toHaveBeenCalledWith('arg1', 'arg2', 123)
  })

  test('returns the result from the wrapped action', () => {
    const mockAction = jest.fn().mockReturnValue('result')
    const actions = { doSomething: mockAction }

    const decorated = decorateActionsWithMetrics(newTabActions, actions)
    const returnValue = decorated.doSomething()

    expect(returnValue).toBe('result')
  })

  test('does not wrap non-function properties', () => {
    const actions = {
      someValue: 'test',
      someNumber: 42,
    }

    const decorated = decorateActionsWithMetrics(newTabActions, actions)

    expect(decorated.someValue).toBe('test')
    expect(decorated.someNumber).toBe(42)
    expect(mockReportEdited).not.toHaveBeenCalled()
  })

  test('wraps multiple actions independently', () => {
    const mockAction1 = jest.fn()
    const mockAction2 = jest.fn()
    const actions = {
      action1: mockAction1,
      action2: mockAction2,
    }

    const decorated = decorateActionsWithMetrics(newTabActions, actions)

    decorated.action1()
    expect(mockReportEdited).toHaveBeenCalledTimes(1)
    expect(mockAction1).toHaveBeenCalledTimes(1)
    expect(mockAction2).not.toHaveBeenCalled()

    decorated.action2()
    expect(mockReportEdited).toHaveBeenCalledTimes(2)
    expect(mockAction2).toHaveBeenCalledTimes(1)
  })
})
