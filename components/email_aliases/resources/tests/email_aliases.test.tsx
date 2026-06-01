// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { ManagePageConnected } from '../email_aliases'
import {
  Alias,
  AliasesUpdate,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

// Mock the email aliases service
class MockEmailAliasesService extends EmailAliasesServiceInterface {
  private observer?: EmailAliasesServiceObserverInterface

  addObserver(observer: EmailAliasesServiceObserverInterface) {
    if (this.observer !== undefined) {
      throw new Error('Expected observer to be undefined')
    }
    this.observer = observer
  }

  notifyObserverAliasesUpdated(aliases: Alias[]) {
    this.observer?.onAliasesUpdated({ aliases } as AliasesUpdate)
  }

  notifyObserverAliasesLoadError(message: string) {
    this.observer?.onAliasesUpdated({ error: message } as AliasesUpdate)
  }

  generateAlias = jest.fn()
  updateAlias = jest.fn()
  deleteAlias = jest.fn()
}

const createBindObserver =
  (emailAliasesService: MockEmailAliasesService) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(observer)
    return () => {}
  }

const mockEmail = 'test@brave.com'

// Test setup helpers
const setupTest = async () => {
  const mockEmailAliasesService = new MockEmailAliasesService()
  const bindObserver = createBindObserver(mockEmailAliasesService)

  render(
    <ManagePageConnected
      authEmail={mockEmail}
      emailAliasesService={mockEmailAliasesService}
      bindObserver={bindObserver}
    />,
  )

  return mockEmailAliasesService
}

const mockAliases = [
  {
    email: 'alias1@brave.com',
    note: 'Test Alias 1',
    domains: undefined,
  },
  {
    email: 'alias2@brave.com',
    note: 'Test Alias 2',
    domains: undefined,
  },
]

const updateAliases = async (
  service: MockEmailAliasesService,
  aliases = mockAliases,
) => {
  await act(() => {
    service.notifyObserverAliasesUpdated(aliases)
  })
}

describe('ManagePageConnected', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('can add new aliases via observer', async () => {
    const service = await setupTest()

    // Add an alias
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })

    // Add more aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })
  })

  it('can remove aliases via observer', async () => {
    const service = await setupTest()

    // Add aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // remove first and last alias
    await updateAliases(service, [
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })

  it('can update aliases via observer', async () => {
    const service = await setupTest()

    // Add aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // swap first/last alias and rename second alias
    await updateAliases(service, [
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
      {
        email: '2.alias@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
    ])

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
    const service = await setupTest()

    // Add aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // clear aliases
    await updateAliases(service, [])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })
})
