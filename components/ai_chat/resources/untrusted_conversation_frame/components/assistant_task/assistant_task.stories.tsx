// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { InferControlsFromArgs } from '../../../../../../.storybook/utils'
import { taskConversationEntries } from '../../../page/stories/story_utils/history'
import AssistantTask from './assistant_task'

type CustomArgs = {
  isActiveTask: boolean
  isGenerating: boolean
}

const args: CustomArgs = {
  isActiveTask: false,
  isGenerating: false,
}

export const _AssistantTask = {
  render: () => {
    return (
      <AssistantTask
        assistantEntries={taskConversationEntries}
        isActiveTask={args.isActiveTask}
        isGenerating={args.isGenerating}
        isLeoModel={true}
      />
    )
  },
}

export default {
  title: 'Chat/Chat',
  component: AssistantTask,
  argTypes: InferControlsFromArgs(args),
  args,
} as Meta<typeof AssistantTask>
