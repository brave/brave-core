// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent } from '@testing-library/react'
import { MainEmailEntryForm } from '../content/email_aliases_signin_page'
import { getLocale } from '$web-common/locale'

import {
  EmailAliasesServiceInterface,
  AuthenticationStatus
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { clickLeoButton } from './test_utils'

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatMessage: (key: string, params: Record<string, string>) => {
    let result = key + " ";
    for (const [key, value] of Object.entries(params)) {
      result += `${key}: ${value} `;
    }
    return result;
  },
  formatLocale: (key: string, params: Record<string, string>) => {
    return key
  }
}))

const mockEmailAliasesService: EmailAliasesServiceInterface = {
  requestAuthentication: jest.fn(),
  cancelAuthenticationOrLogout: jest.fn(),
  generateAlias: jest.fn(),
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  addObserver: jest.fn(),
  removeObserver: jest.fn()
}

describe('MainEmailEntryForm', () => {

  it('handles sign up via button click', async () => {
    const mockAuthEmail = 'test@example.com'

    render(<MainEmailEntryForm
      authState={
        { status: AuthenticationStatus.kUnauthenticated,
        email: mockAuthEmail }}
      emailAliasesService={mockEmailAliasesService} />)

    const signUpButton = screen.getByText(getLocale('emailAliasesGetLoginLinkButton'))
    const emailInput = screen.getByPlaceholderText(getLocale('emailAliasesEmailAddressPlaceholder'))
    fireEvent.change(emailInput, { target: { value: mockAuthEmail } })
    clickLeoButton(signUpButton)
    expect(mockEmailAliasesService.requestAuthentication).toHaveBeenCalledWith(mockAuthEmail)
  })

})
