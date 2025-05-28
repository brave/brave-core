// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { ManagePageConnected } from '../email_aliases'
import {
  Alias,
  AuthenticationStatus,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatMessage: (key: string) => {
    return key
  },
  formatLocale: (key: string) => {
    return key
  }
}))

// Mock the email aliases service
class MockEmailAliasesService extends EmailAliasesServiceInterface {
  private observer?: EmailAliasesServiceObserverInterface

  addObserver(observer: EmailAliasesServiceObserverInterface) {
    expect(this.observer).toBeUndefined()
    this.observer = observer
  }

  notifyObserverAliasesUpdated(aliases: Alias[]) {
    expect(this.observer).toBeDefined()
    this.observer?.onAliasesUpdated(aliases)
  }

  notifyObserverAuthStateChanged(status: AuthenticationStatus,
                                 email: string,
                                 errorMessage?: string) {
    expect(this.observer).toBeDefined()
    this.observer?.onAuthStateChanged(
      { status, email, errorMessage })
  }

  removeObserver() { }

  generateAlias = jest.fn()
  updateAlias = jest.fn()
  deleteAlias = jest.fn()
  requestAuthentication = jest.fn()
  cancelAuthentication = jest.fn()
  logout = jest.fn()
}

const createBindObserver =
  (emailAliasesService: MockEmailAliasesService) =>
    (observer: EmailAliasesServiceObserverInterface) => {
      emailAliasesService.addObserver(observer)
      return () => { }
    }

const mockEmail = 'test@brave.com'

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
      expect(screen.getByText(
        'emailAliasesConnectingToBraveAccount')).toBeInTheDocument()
    })
  })

  it('shows sign up form when no email is available', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kUnauthenticated,
        ''
      )
    })

    await waitFor(() => {
      expect(screen.getByText('emailAliasesSignInOrCreateAccount'))
        .toBeInTheDocument()
      expect(screen.getByPlaceholderText('emailAliasesEmailAddressPlaceholder'))
        .toBeInTheDocument()
    })
  })

  it('shows main view when email is available', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
      expect(screen.getByText('emailAliasesBraveAccount'))
        .toBeInTheDocument()
      expect(screen.getByText('emailAliasesSignOut')).toBeInTheDocument()
    })
  })

  it('shows verification pending view', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await act(async () => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticating,
        ''
      )
    })

    await waitFor(() => {
      expect(screen.getByText('emailAliasesLoginEmailOnTheWay'))
        .toBeInTheDocument()
      expect(screen.getByText('emailAliasesClickOnSecureLogin'))
        .toBeInTheDocument()
      expect(screen.getByText('emailAliasesDontSeeEmail'))
        .toBeInTheDocument()
    })
  })

  it('can add new aliases via observer', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Log in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // Add an alias
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })

    // Add more aliases
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }, {
        email: 'alias3@brave.com',
        name: 'Test Alias 3'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })
  })

  it('can remove aliases via observer', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Log in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // Add aliases
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }, {
        email: 'alias3@brave.com',
        name: 'Test Alias 3'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // remove first and last alias
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })

  it('can update aliases via observer', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Log in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // Add aliases
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }, {
        email: 'alias3@brave.com',
        name: 'Test Alias 3'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // swap first/last alias and rename second alias
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias3@brave.com',
        name: 'Test Alias 3'
      }, {
        email: '2.alias@brave.com',
        name: 'Test Alias 2'
      }, {
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }])
    })

    // Note: We don't actually care about ordering so we don't check that the
    // order of first/last was swapped.
    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('2.alias@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })
  })

  it('can clear aliases via observer', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Log in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // Add aliases
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }, {
        email: 'alias3@brave.com',
        name: 'Test Alias 3'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // clear aliases
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })

  it('does not show aliases when logged out', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Notify of aliases, while not logged in.
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }])
    })

    // Shouldn't show the aliases in the UI.
    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
    })
  })

  it('hides aliases when logged out', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Notify logged in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // Notify of aliases
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }])
    })

    // Notify logged out
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kUnauthenticated,
        ''
      )
    })

    // Shouldn't be showing the aliases in the UI - we're logged out.
    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
    })
  })

  it('does not show aliases which were added before logging in', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }])
    })

    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
    })
  })

  it('does not show aliases from previous logins', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    // Log in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // Notify of aliases.
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesUpdated([{
        email: 'alias1@brave.com',
        name: 'Test Alias 1'
      }, {
        email: 'alias2@brave.com',
        name: 'Test Alias 2'
      }])
    })

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
    })

    // Log out
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kUnauthenticated,
        ''
      )
    })

    // Log back in
    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticated,
        mockEmail
      )
    })

    // We shouldn't be showing the aliases from the previous login
    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
    })
  })

  it('shows error message when auth fails', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()

    await act(async () => {
      render(<ManagePageConnected
        emailAliasesService={mockEmailAliasesService}
        bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await act(() => {
      mockEmailAliasesService.notifyObserverAuthStateChanged(
        AuthenticationStatus.kAuthenticating,
        'test@brave.com',
        'mockErrorMessage'
      )
    })

    await waitFor(() => {
      expect(screen.getByText(/mockErrorMessage/)).toBeInTheDocument()
    })
  })

})
