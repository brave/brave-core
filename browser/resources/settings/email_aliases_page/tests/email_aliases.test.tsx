// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { ManagePageConnected } from '../email_aliases'
import { localeRegex } from './test_utils'
import {
  AuthenticationStatus,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
  Alias
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatMessage: (key: string, params: Record<string, string>) => {
    return key
  },
  formatLocale: (key: string, params: Record<string, string>) => {
    return key
  }
}))

// Mock the email aliases service
class MockEmailAliasesService extends EmailAliasesServiceInterface {
  private observers: Set<EmailAliasesServiceObserverInterface> = new Set()
  private aliases: Map<string, Alias> = new Map()
  private authStatus: AuthenticationStatus = AuthenticationStatus.kStartup
  private authEmail: string = ''

  private notifyObserversAliasesUpdated = () => {
    this.observers.forEach((observer) => {
      observer.onAliasesUpdated(Array.from(this.aliases.values()))
    })
  }

  private notifyObserversAuthStateChanged = () => {
    this.observers.forEach((observer) => {
      observer.onAuthStateChanged({
        status: this.authStatus,
        email: this.authEmail
      })
    })
  }
  addObserver(observer: EmailAliasesServiceObserverInterface) {
    this.observers.add(observer)
    this.notifyObserversAliasesUpdated()
    this.notifyObserversAuthStateChanged()
  }

  removeObserver(observer: EmailAliasesServiceObserverInterface) {
    this.observers.delete(observer)
  }

  generateAlias = jest.fn()
  updateAlias = (aliasEmail: string, note?: string | null) => {
    this.aliases.set(aliasEmail, {
      email: aliasEmail,
      note: note !== null ? note : undefined,
      domains: undefined,
    })
    this.notifyObserversAliasesUpdated()
  }
  deleteAlias = (aliasEmail: string) => {
    this.aliases.delete(aliasEmail)
    this.notifyObserversAliasesUpdated()
  }
  requestAuthentication = () => {
    this.authStatus = AuthenticationStatus.kAuthenticating
    this.authEmail = 'test@brave.com'
    this.notifyObserversAuthStateChanged()
  }
  cancelAuthenticationOrLogout = () => {
    this.authStatus = AuthenticationStatus.kUnauthenticated
    this.authEmail = ''
    this.notifyObserversAuthStateChanged()
  }
  simulateAuthenticated = () => {
    this.authStatus = AuthenticationStatus.kAuthenticated
    this.authEmail = 'test@brave.com'
    this.notifyObserversAuthStateChanged()
  }
}

const createBindObserver =
  (emailAliasesService: MockEmailAliasesService) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(observer)
    return () => {
      emailAliasesService.removeObserver(observer)
    }
  }

const mockEmail = 'test@brave.com'
const mockAliases = [
  {
    email: 'alias1@brave.com',
    note: 'Test Alias 1',
    domains: undefined,
  }
]

describe('ManagePageConnected', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('shows loading state initially', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()
    await act(async () => {
      render(<ManagePageConnected
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })
    await waitFor(() => {
      expect(document.querySelector('leo-progressring'))
        .toBeInTheDocument()
      expect(screen.getByText(localeRegex(
        'emailAliasesConnectingToBraveAccount'))).toBeInTheDocument()
    })
  })

  it('shows sign up form when no email is available', async () => {
    const mockEmailAliasesService =
      new MockEmailAliasesService()

    await act(async () => {
      mockEmailAliasesService.cancelAuthenticationOrLogout()
    })

    await act(async () => {
      render(<ManagePageConnected
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesSignInOrCreateAccount')))
        .toBeInTheDocument()
      expect(screen.getByPlaceholderText(localeRegex(
        'emailAliasesEmailAddressPlaceholder'))).toBeInTheDocument()
    })
  })

  it('shows main view when email is available', async () => {

    const mockEmailAliasesService =
      new MockEmailAliasesService()

    await act(async () => {
      mockEmailAliasesService.simulateAuthenticated()
    })

    await act(async () => {
      render(<ManagePageConnected
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesBraveAccount')))
        .toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesSignOut')))
        .toBeInTheDocument()
    })
  })

  it('shows verification pending view', async () => {
    const mockEmailAliasesService =
      new MockEmailAliasesService()

    await act(async () => {
      mockEmailAliasesService.requestAuthentication()
    })

    await act(async () => {
      render(<ManagePageConnected
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesLoginEmailOnTheWay')))
        .toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesClickOnSecureLogin')))
        .toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesDontSeeEmail')))
        .toBeInTheDocument()
    })
  })

  it('updates the list of aliases according to the observer', async () => {
    const mockEmailAliasesService =
      new MockEmailAliasesService()

    await act(async () => {
      mockEmailAliasesService.simulateAuthenticated()
    })

    await act(async () => {
      render(<ManagePageConnected
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await act(async () => {
      // Add an alias
      mockEmailAliasesService.updateAlias('alias1@brave.com', 'Test Alias 1')
    })

    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
      expect(screen.getByText(/alias1@brave\.com/)).toBeInTheDocument()
      expect(screen.getByText(/Test Alias 1/)).toBeInTheDocument()
    })

    await act(async () => {
      // Add a second alias
      mockEmailAliasesService.updateAlias('alias2@brave.com', 'Test Alias 2')
    })

    await waitFor(() => {
      expect(screen.getByText(/alias2@brave\.com/)).toBeInTheDocument()
      expect(screen.getByText(/Test Alias 2/)).toBeInTheDocument()
    })

    await act(async () => {
      // Update the second alias
      mockEmailAliasesService.updateAlias(
        'alias2@brave.com', 'Test_Alias_2_updated')
    })

    await waitFor(() => {
      expect(screen.queryByText(/Test Alias 2/)).not.toBeInTheDocument()
      expect(screen.getByText(/Test_Alias_2_updated/)).toBeInTheDocument()
    })

    await act(async () => {
      // Delete the first alias
      mockEmailAliasesService.deleteAlias('alias1@brave.com')
    })

    await waitFor(() => {
      expect(screen.queryByText(/alias1@brave\.com/)).not.toBeInTheDocument()
      expect(screen.queryByText(/Test Alias 1/)).not.toBeInTheDocument()
      expect(screen.getByText(/alias2@brave\.com/)).toBeInTheDocument()
      expect(screen.getByText(/Test_Alias_2_updated/)).toBeInTheDocument()
    })
  })
})
