// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { InferControlsFromArgs } from '../../../../../../.storybook/utils'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import ToolPermissionChallenge from './tool_permission_challenge'

type CustomArgs = {
  isInteractive: boolean
  hasAssessment: boolean
  hasPlan: boolean
}

const args: CustomArgs = {
  isInteractive: true,
  hasAssessment: true,
  hasPlan: false,
}

export const _ToolPermissionChallenge = {
  render: (args: CustomArgs) => {
    const toolUseEvent: Mojom.ToolUseEvent = {
      toolName: 'toolName',
      id: 'toolId',
      argumentsJson: 'toolArguments',
      output: undefined,
      permissionChallenge: {
        assessment: args.hasAssessment
          ? 'This is not at all what you asked for. The agent may have been misled by untrusted content.'
          : undefined,
        plan: args.hasPlan
          ? 'I am going to group your tabs by category.'
          : undefined,
      },
    }
    return (
      <MockContext>
        <ToolPermissionChallenge
          isInteractive={args.isInteractive}
          toolUseEvent={toolUseEvent}
          toolLabel={
            args.hasPlan
              ? 'Managing your tabs'
              : 'Navigating to https://www.example.com/path/to/page'
          }
        />
      </MockContext>
    )
  },
}

export default {
  title: 'Chat/Chat',
  component: ToolPermissionChallenge,
  argTypes: InferControlsFromArgs(args),
  args,
} as Meta<typeof ToolPermissionChallenge>
