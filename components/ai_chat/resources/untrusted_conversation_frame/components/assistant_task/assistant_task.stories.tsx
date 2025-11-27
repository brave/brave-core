// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import { InferControlsFromArgs } from '../../../../../../.storybook/utils'
import * as Mojom from '../../../common/mojom'
import { taskConversationEntries } from '../../../page/stories/story_utils/history'
import MockContext from '../../mock_untrusted_conversation_context'
import AssistantTask from './assistant_task'
import { getKeysForMojomEnum } from '$web-common/mojomUtils'

type CustomArgs = {
  isActiveTask: boolean
  isGenerating: boolean
  isToolExecuting: boolean
  hasContentThumbnail: boolean
  toolUseTaskState: keyof typeof Mojom.TaskState
}

const args: CustomArgs = {
  isActiveTask: false,
  isGenerating: false,
  isToolExecuting: false,
  hasContentThumbnail: false,
  toolUseTaskState: 'kNone' satisfies keyof typeof Mojom.TaskState,
}

export const _AssistantTask = {
  render: (args: CustomArgs) => {
    return (
      <MockContext
        contentTaskTabId={args.hasContentThumbnail ? 1 : undefined}
        isGenerating={args.isGenerating}
        isToolExecuting={args.isToolExecuting}
        toolUseTaskState={Mojom.TaskState[args.toolUseTaskState]}
        uiObserver={
          {
            thumbnailUpdated: {
              addListener: (
                listener: (tabId: number, dataURI: string) => void,
              ) => {
                listener(
                  1,
                  'data:image/jpeg;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAKCAIAAAACUFjqAAAAGUlEQVR4nGKpOP+JATdgwiM3gqUBAQAA//+9LAJQNrjO2wAAAABJRU5ErkJggg==',
                )
              },
            },
            removeListener: () => true,
          } as unknown as Mojom.UntrustedUICallbackRouter
        }
      >
        <AssistantTask
          assistantEntries={taskConversationEntries}
          isActiveTask={args.isActiveTask}
          isLeoModel={true}
        />
      </MockContext>
    )
  },
}

export default {
  title: 'Chat/Chat',
  component: AssistantTask,
  argTypes: {
    ...InferControlsFromArgs(args),
    toolUseTaskState: {
      options: getKeysForMojomEnum(Mojom.TaskState),
      control: { type: 'select' },
    },
  },
  args,
} as Meta<typeof AssistantTask>
