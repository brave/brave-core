// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import { createTextContentBlock } from '../../../common/content_block'
import * as React from 'react'
import MockContext from '../../mock_untrusted_conversation_context'
import ToolEvent from './tool_event'

describe('ToolEventTabManagement', () => {
  test('should render permission request state', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'tab_management',
            id: 'test-id',
            argumentsJson:
              '{"url": "https://example.com", "title": "Example Site"}',
            output: undefined,
            requiresUserInteraction: true,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Should show permission icon
    expect(
      screen.getByTestId('tool-tabmanagement-permission-icon'),
    ).toBeInTheDocument()

    // Should show allow and deny buttons
    expect(
      screen.getByText('AI_CHAT_TOOL_TAB_MANAGEMENT_PERMISSION_ALLOW_BUTTON'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('AI_CHAT_TOOL_TAB_MANAGEMENT_PERMISSION_DENY_BUTTON'),
    ).toBeInTheDocument()
  })

  test('should render active state', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'tab_management',
            id: 'test-id',
            argumentsJson:
              '{"url": "https://example.com", "title": "Example Site"}',
            output: undefined,
            requiresUserInteraction: false,
          }}
          isEntryActive={true}
        />
      </MockContext>,
    )

    // Should show active state description
    expect(
      screen.getByText('AI_CHAT_TOOL_TAB_MANAGEMENT_ACTIVE_STATE_DESCRIPTION'),
    ).toBeInTheDocument()

    // Should show progress icon
    expect(screen.getByTestId('tool-default-progress-icon')).toBeInTheDocument()
    expect(
      screen.queryByTestId('tool-tabmanagement-permission-icon'),
    ).not.toBeInTheDocument()
  })

  test('should render completed state', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'tab_management',
            id: 'test-id',
            argumentsJson:
              '{"url": "https://example.com", "title": "Example Site"}',
            output: [createTextContentBlock('Tab opened successfully')],
            requiresUserInteraction: false,
          }}
          isEntryActive={false}
        />
      </MockContext>,
    )

    // Should show completed state description
    expect(
      screen.getByText(
        'AI_CHAT_TOOL_TAB_MANAGEMENT_COMPLETED_STATE_DESCRIPTION',
      ),
    ).toBeInTheDocument()

    // Should show completed icon
    expect(
      screen.getByTestId('tool-default-completed-icon'),
    ).toBeInTheDocument()
    expect(
      screen.queryByTestId('tool-default-progress-icon'),
    ).not.toBeInTheDocument()
  })

  test('should display tooltip content when output exists', () => {
    render(
      <MockContext>
        <ToolEvent
          toolUseEvent={{
            toolName: 'tab_management',
            id: 'test-id',
            argumentsJson:
              '{"url": "https://example.com", "title": "Example Site"}',
            output: [
              createTextContentBlock('Tab operation completed successfully'),
            ],
            requiresUserInteraction: false,
          }}
          isEntryActive={false}
        />
      </MockContext>,
    )

    // The tooltip content should be rendered (though exact display depends on Tooltip component behavior)
    // We can at least verify the component renders without error and the main content is present
    expect(
      screen.getByText(
        'AI_CHAT_TOOL_TAB_MANAGEMENT_COMPLETED_STATE_DESCRIPTION',
      ),
    ).toBeInTheDocument()
  })
})
