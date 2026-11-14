// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { InferControlsFromArgs } from '$storybook/utils'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import { getToolLabel } from '../assistant_response/get_tool_label'
import ToolPermissionChallenge from './tool_permission_challenge'

type CustomArgs = {
  isInteractive: boolean
  hasAssessment: boolean
  hasPlan: boolean
  hasImplications: boolean
  implicationsKind: 'tabs' | 'tabsWithPageContent' | 'history'
  isWebTool: boolean
  argumentsJson: string
}

const args: CustomArgs = {
  isInteractive: true,
  hasAssessment: true,
  hasPlan: false,
  hasImplications: true,
  implicationsKind: 'tabs',
  isWebTool: false,
  argumentsJson: JSON.stringify({
    action: 'group',
    group_title: 'Recipes',
    tab_ids: [12, 13, 14],
    options: { collapse: true, color: 'blue' },
  }),
}

export const _ToolPermissionChallenge = {
  render: (args: CustomArgs) => {
    const implications = {
      tabs: getLocale(
        S.CHAT_UI_TOOL_TAB_MANAGEMENT_PERMISSION_IMPLICATIONS_MARKDOWN,
      ),
      tabsWithPageContent: getLocale(
        S.CHAT_UI_TOOL_TAB_MANAGEMENT_PAGE_CONTENT_PERMISSION_IMPLICATIONS,
      ),
      history: getLocale(
        S.CHAT_UI_TOOL_SEMANTIC_HISTORY_SEARCH_PERMISSION_IMPLICATIONS_MARKDOWN,
      ),
    }[args.implicationsKind]
    // Mirrors what tool_event.tsx does before rendering this component: the
    // arguments are parsed from LLM output, so parsing can fail.
    let toolInput: any = null
    try {
      toolInput = JSON.parse(args.argumentsJson)
    } catch (e) {
      toolInput = null
    }

    const toolUseEvent: Mojom.ToolUseEvent = {
      toolName: args.isWebTool
        ? 'web_example_com_get_stock_price'
        : args.implicationsKind === 'history'
          ? Mojom.SEMANTIC_HISTORY_SEARCH_TOOL_NAME
          : args.hasImplications
            ? Mojom.TAB_MANAGEMENT_TOOL_NAME
            : Mojom.CODE_EXECUTION_TOOL_NAME,
      id: 'toolId',
      argumentsJson: args.argumentsJson,
      output: undefined,
      isServerResult: false,
      artifacts: undefined,
      permissionChallenge: {
        assessment: args.hasAssessment
          ? 'This is not at all what you asked for. The agent may have been misled by untrusted content.'
          : undefined,
        plan: args.hasPlan
          ? 'I am going to group your tabs by category.'
          : undefined,
        title: args.isWebTool ? 'Tool permission request' : undefined,
        description: args.isWebTool
          ? 'Brave AI would like to execute **get_stock_price** on **https://example.com**'
          : undefined,
        implications:
          args.hasImplications && !args.isWebTool ? implications : undefined,
        supportsAllowSession: args.isWebTool,
      },
    }
    return (
      <MockContext>
        <ToolPermissionChallenge
          isInteractive={args.isInteractive}
          toolUseEvent={toolUseEvent}
          toolInput={toolInput}
          toolLabel={getToolLabel(toolUseEvent.toolName, toolInput)}
        />
      </MockContext>
    )
  },
}

export const HistorySearch = {
  ..._ToolPermissionChallenge,
  args: {
    implicationsKind: 'history',
    argumentsJson: '{"query":"lighthouses"}',
  },
}

export const TabsWithPageContent = {
  ..._ToolPermissionChallenge,
  args: { implicationsKind: 'tabsWithPageContent' },
}

export default {
  title: 'AI Chat/Permission Challenge',
  component: ToolPermissionChallenge,
  argTypes: {
    ...InferControlsFromArgs(args),
    implicationsKind: {
      control: 'select',
      options: ['tabs', 'tabsWithPageContent', 'history'],
    },
  },
  args,
} as Meta<typeof ToolPermissionChallenge>
