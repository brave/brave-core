// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { render, screen, fireEvent } from '@testing-library/react'
import {
  AIChatContext,
  AIChatReactContext,
  defaultContext as defaultAIChatContext,
} from '../../state/ai_chat_context'
import {
  ConversationReactContext,
  ConversationContext,
  defaultContext as defaultConversationContext,
} from '../../state/conversation_context'
import Attachments from './index'
import {
  AssociatedContent,
  ContentType,
} from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'

const MockContext = (
  props: React.PropsWithChildren<Partial<AIChatContext & ConversationContext>>,
) => {
  const mockContext = {
    ...defaultConversationContext,
    unassociatedTabs: [],
    associatedContentInfo: [],
    conversationUuid: undefined,
    ...props,
  }

  return (
    <AIChatReactContext.Provider
      value={{
        ...defaultAIChatContext,
        ...props,
      }}
    >
      <ConversationReactContext.Provider value={mockContext}>
        {props.children}
      </ConversationReactContext.Provider>
    </AIChatReactContext.Provider>
  )
}

const mockTabs = [
  {
    contentId: 1,
    title: 'Google Search',
    url: { url: 'https://google.com' },
    id: 1,
  },
  {
    contentId: 2,
    title: 'GitHub - Brave Browser',
    url: { url: 'https://github.com/brave/brave-browser' },
    id: 2,
  },
  {
    contentId: 3,
    title: 'Stack Overflow',
    url: { url: 'https://stackoverflow.com' },
    id: 3,
  },
]

const mockAssociatedContent: AssociatedContent[] = [
  {
    conversationTurnUuid: undefined,
    contentId: 1,
    title: 'Google Search',
    url: { url: 'https://google.com' },
    contentType: ContentType.PageContent,
    contentUsedPercentage: 0.5,
    uuid: 'uuid-1',
  },
]

describe('Attachments Component', () => {
  const mockSetAttachmentsDialog = jest.fn()
  const mockAssociateTab = jest.fn()
  const mockDisassociateContent = jest.fn()

  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('renders the attachments dialog with correct title', () => {
    render(
      <MockContext
        unassociatedTabs={mockTabs}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    // The title will be the raw locale key in tests
    expect(
      screen.getByText('CHAT_UI_ATTACHMENTS_TABS_TITLE'),
    ).toBeInTheDocument()
  })

  it('renders the close button', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    const closeButton = container.querySelector('leo-button')
    expect(closeButton).toBeInTheDocument()
  })

  it('calls setAttachmentsDialog(null) when close button is clicked', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    const closeButton = container.querySelector('leo-button')!
    fireEvent.click(closeButton)

    expect(mockSetAttachmentsDialog).toHaveBeenCalledWith(null)
  })

  it('displays all unassociated tabs', () => {
    render(
      <MockContext
        unassociatedTabs={mockTabs}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    expect(screen.getByText('Google Search')).toBeInTheDocument()
    expect(screen.getByText('GitHub - Brave Browser')).toBeInTheDocument()
    expect(screen.getByText('Stack Overflow')).toBeInTheDocument()
  })

  it('displays tab URLs', () => {
    render(
      <MockContext
        unassociatedTabs={mockTabs}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    expect(screen.getByText('https://google.com')).toBeInTheDocument()
    expect(
      screen.getByText('https://github.com/brave/brave-browser'),
    ).toBeInTheDocument()
    expect(screen.getByText('https://stackoverflow.com')).toBeInTheDocument()
  })

  it('displays favicons for each tab', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    const favicons = container.querySelectorAll('img.icon')
    expect(favicons).toHaveLength(3)

    expect(favicons[0]).toHaveAttribute(
      'src',
      expect.stringContaining('google.com'),
    )
    expect(favicons[1]).toHaveAttribute(
      'src',
      expect.stringContaining('github.com'),
    )
    expect(favicons[2]).toHaveAttribute(
      'src',
      expect.stringContaining('stackoverflow.com'),
    )
  })

  it('shows checkboxes in unchecked state for unassociated tabs', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        associatedContentInfo={[]}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    const checkboxes = container.querySelectorAll('leo-checkbox')
    expect(checkboxes).toHaveLength(3)

    checkboxes.forEach((checkbox) => {
      expect(checkbox).toHaveProperty('checked', false)
    })
  })

  it('shows checkboxes in checked state for associated tabs', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        associatedContentInfo={mockAssociatedContent}
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    const checkboxes = container.querySelectorAll('leo-checkbox')
    expect(checkboxes).toHaveLength(3)

    // First tab should be checked (associated), others unchecked
    expect(checkboxes[0]).toHaveProperty('checked', true)
    expect(checkboxes[1]).toHaveProperty('checked', false)
    expect(checkboxes[2]).toHaveProperty('checked', false)
  })

  it('has proper checkbox structure for association', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        associatedContentInfo={[]}
        conversationUuid='test-conversation'
        uiHandler={
          // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
          {
            ...defaultAIChatContext.uiHandler,
            associateTab: mockAssociateTab,
          } as any
        }
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    const checkboxes = container.querySelectorAll('leo-checkbox')
    expect(checkboxes).toHaveLength(3)

    // Verify that each checkbox has the expected content structure
    const firstCheckbox = checkboxes[0]
    expect(firstCheckbox).toContainHTML('Google Search')
    expect(firstCheckbox).toContainHTML('https://google.com')
  })

  it('properly renders associated content info', () => {
    const { container } = render(
      <MockContext
        unassociatedTabs={mockTabs}
        associatedContentInfo={mockAssociatedContent}
        conversationUuid='test-conversation'
        uiHandler={
          // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
          {
            ...defaultAIChatContext.uiHandler,
            disassociateContent: mockDisassociateContent,
          } as any
        }
        setAttachmentsDialog={mockSetAttachmentsDialog}
      >
        <Attachments />
      </MockContext>,
    )

    // First tab should be in the associated content
    const firstCheckbox = container.querySelectorAll('leo-checkbox')[0]
    expect(firstCheckbox).toContainHTML('Google Search')
    expect(firstCheckbox).toHaveProperty('checked', true)
  })

  describe('Search functionality', () => {
    it('renders search input', () => {
      const { container } = render(
        <MockContext
          unassociatedTabs={mockTabs}
          setAttachmentsDialog={mockSetAttachmentsDialog}
        >
          <Attachments />
        </MockContext>,
      )

      const searchInput = container.querySelector('leo-input')
      expect(searchInput).toBeInTheDocument()
    })

    it('renders all tabs when no search filter is applied', () => {
      render(
        <MockContext
          unassociatedTabs={mockTabs}
          setAttachmentsDialog={mockSetAttachmentsDialog}
        >
          <Attachments />
        </MockContext>,
      )

      // All tabs should be visible when no search is active
      expect(screen.getByText('Google Search')).toBeInTheDocument()
      expect(screen.getByText('GitHub - Brave Browser')).toBeInTheDocument()
      expect(screen.getByText('Stack Overflow')).toBeInTheDocument()
    })

    it('shows search input with correct styling', () => {
      const { container } = render(
        <MockContext
          unassociatedTabs={mockTabs}
          setAttachmentsDialog={mockSetAttachmentsDialog}
        >
          <Attachments />
        </MockContext>,
      )

      const searchInput = container.querySelector('leo-input')!
      expect(searchInput).toHaveClass('searchBox')
    })

    it('shows search icon in input', () => {
      const { container } = render(
        <MockContext
          unassociatedTabs={mockTabs}
          setAttachmentsDialog={mockSetAttachmentsDialog}
        >
          <Attachments />
        </MockContext>,
      )

      const searchIcon = container.querySelector(
        'leo-input leo-icon[slot="left-icon"]',
      )
      expect(searchIcon).toBeInTheDocument()
    })
  })

  describe('Empty states', () => {
    it('shows no results message when there are no unassociated tabs', () => {
      render(
        <MockContext
          unassociatedTabs={[]}
          setAttachmentsDialog={mockSetAttachmentsDialog}
        >
          <Attachments />
        </MockContext>,
      )

      expect(
        screen.getByText('CHAT_UI_ATTACHMENTS_SEARCH_NO_RESULTS'),
      ).toBeInTheDocument()
      // Should not show suggestion text when there are no tabs at all
      expect(
        screen.queryByText('CHAT_UI_ATTACHMENTS_SEARCH_NO_RESULTS_SUGGESTION'),
      ).not.toBeInTheDocument()
    })

    it('renders properly with empty tabs list', () => {
      const { container } = render(
        <MockContext
          unassociatedTabs={[]}
          setAttachmentsDialog={mockSetAttachmentsDialog}
        >
          <Attachments />
        </MockContext>,
      )

      // Should still render the basic structure
      expect(
        screen.getByText('CHAT_UI_ATTACHMENTS_TABS_TITLE'),
      ).toBeInTheDocument()
      expect(container.querySelector('leo-input')).toBeInTheDocument()

      // But no tab items should be present
      expect(container.querySelectorAll('leo-checkbox')).toHaveLength(0)
    })
  })
})
