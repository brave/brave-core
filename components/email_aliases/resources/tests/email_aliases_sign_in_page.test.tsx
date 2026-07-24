// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'
import { SignInPage } from '../content/email_aliases_manage_page'
import { useBraveAccountState } from '../content/use_email_aliases'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'

jest.mock('../content/use_email_aliases', () => ({
  ...jest.requireActual('../content/use_email_aliases'),
  useBraveAccountState: jest.fn(),
}))

const mockUseBraveAccountState = jest.mocked(useBraveAccountState)

const defineStubElement = (name: string) => {
  if (!customElements.get(name)) {
    customElements.define(name, class extends HTMLElement {})
  }
}

const makeLoggedInAccountState = (): AccountState =>
  ({ loggedIn: { email: 'test@brave.com' } }) as AccountState

const makeLoggedOutAccountState = (): AccountState =>
  ({ loggedOut: {} }) as AccountState

describe('SignInPage autofill suggestion toggle visibility', () => {
  beforeAll(() => {
    defineStubElement('settings-brave-account-row')
  })

  beforeEach(() => {
    jest.clearAllMocks()
  })

  it.each([
    ['account state is unknown', undefined, false],
    ['the user is logged out', makeLoggedOutAccountState(), false],
    ['the user is logged in', makeLoggedInAccountState(), true],
  ])(
    'notifies onLoggedInChange when %s',
    (_label, accountState, expectedLoggedIn) => {
      mockUseBraveAccountState.mockReturnValue(accountState)
      const onLoggedInChange = jest.fn()

      render(<SignInPage onLoggedInChange={onLoggedInChange} />)

      expect(onLoggedInChange).toHaveBeenCalledWith(expectedLoggedIn)
    },
  )
})
