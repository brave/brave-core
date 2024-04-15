/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { readCommandMessage } from './message_data_reader';

describe('readCommandMessage', () => {
  const openRewardsOnboardingCommandId = 1

  it('throws if arg is invalid', () => {
    expect(() => readCommandMessage(undefined)).toThrow()
    expect(() => readCommandMessage(null)).toThrow()
    expect(() => readCommandMessage('str')).toThrow()
    expect(() => readCommandMessage(0)).toThrow()
  })

  it('throws if messageType is invalid', () => {
    expect(() => readCommandMessage({})).toThrow()
    expect(() => readCommandMessage({
      messageType: 'bowser-command',
      command: 'open-rewards-onboarding'
    })).toThrow()
  })

  it('throws if command is unrecognized', () => {
    expect(() => readCommandMessage({
      messageType: 'browser-command',
      command: 'open-shield-settings'
    })).toThrow()
  })

  it('returns correctly mapped data', () => {
    expect(readCommandMessage({
      messageType: 'browser-command',
      command: 'open-rewards-onboarding',
    })).toEqual({
      command: openRewardsOnboardingCommandId
    })
  })
})
