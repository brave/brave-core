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

const mockPrefs = {}

const makeLoggedInAccountState = (): AccountState =>
  ({ loggedIn: { email: 'test@brave.com' } }) as AccountState

const makeLoggedOutAccountState = (): AccountState =>
  ({ loggedOut: {} }) as AccountState

describe('SignInPage autofill suggestion toggle', () => {
  beforeAll(() => {
    defineStubElement('settings-brave-account-row')
    defineStubElement('settings-email-aliases-autofill-toggle')
  })

  beforeEach(() => {
    jest.clearAllMocks()
  })

  it.each([
    ['account state is unknown', undefined],
    ['the user is logged out', makeLoggedOutAccountState()],
  ])('does not render autofill toggle when %s', (_label, accountState) => {
    mockUseBraveAccountState.mockReturnValue(accountState)

    const { container } = render(<SignInPage prefs={mockPrefs} />)

    expect(
      container.querySelector('settings-email-aliases-autofill-toggle'),
    ).not.toBeInTheDocument()
  })

  it('renders autofill toggle when the user is logged in', () => {
    mockUseBraveAccountState.mockReturnValue(makeLoggedInAccountState())

    const { container } = render(<SignInPage prefs={mockPrefs} />)

    expect(
      container.querySelector('settings-email-aliases-autofill-toggle'),
    ).toBeInTheDocument()
  })
})
