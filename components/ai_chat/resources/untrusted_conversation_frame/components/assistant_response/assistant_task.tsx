// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Tabs from '@brave/leo/react/tabs'
import TabItem from '@brave/leo/react/tabItem'
import { getLocale } from '$web-common/locale'
import API from '../../untrusted_conversation_frame_api'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantResponse from '.'
import ToolEvent from './tool_event'
import styles from './assistant_task.module.scss'

interface Props {
  // Entries that make up the task loop
  assistantEntries: Mojom.ConversationTurn[]
  isActiveTask: boolean
  isGenerating: boolean
}

const interactiveToolNames = [Mojom.USER_CHOICE_TOOL_NAME]

const importantToolNames = [Mojom.NAVIGATE_TOOL_NAME, Mojom.TODO_TOOL_NAME]

const skipStepsToolNames = [Mojom.TODO_TOOL_NAME]

export default function AssistantTask(props: Props) {
  const [showSteps, setShowSteps] = React.useState(false)
  const [taskThumbnail, setTaskThumbnail] = React.useState<string>()
  const conversationContext = useUntrustedConversationContext()

  React.useEffect(() => {
    if (!conversationContext.contentTaskTabId) {
      return
    }
    const id = API.getInstance().uiObserver.thumbnailUpdated.addListener(
      (tabId: number, dataURI: string) => {
        console.error(dataURI)
        if (
          tabId === conversationContext.contentTaskTabId
          && props.isActiveTask
        ) {
          setTaskThumbnail(dataURI)
        }
      },
    )

    return () => {
      API.getInstance().uiObserver.thumbnailUpdated.removeListener(id)
    }
  }, [props.isActiveTask, conversationContext.contentTaskTabId])

  return (
    <div className={styles.task}>
      <Tabs
        onChange={() => setShowSteps(!showSteps)}
        value={showSteps ? 'steps' : 'progress'}
        size='medium'
      >
        <TabItem value='progress'>
          {getLocale(S.AI_CHAT_TASK_PROGRESS_LABEL)}
        </TabItem>
        <TabItem value='steps'>{getLocale(S.AI_CHAT_TASK_STEPS_LABEL)}</TabItem>
      </Tabs>
      <div className={styles.taskContent}>
        <div className={styles.taskData}>
          {showSteps
            && props.assistantEntries.map((entry, index) => {
              // Show all the tool use events, excluding todo
              const lastEntryEdit = entry.edits?.at(-1) ?? entry
              const toolUseEvents =
                lastEntryEdit.events?.filter(
                  (event) =>
                    !!event
                    && event.toolUseEvent
                    && !skipStepsToolNames.includes(
                      event.toolUseEvent?.toolName ?? '',
                    ),
                ) ?? []
              const isLastEntry = index === props.assistantEntries.length - 1
              return (
                <AssistantResponse
                  key={entry.uuid || index}
                  events={toolUseEvents}
                  isEntryInteractivityAllowed={
                    props.isActiveTask && isLastEntry
                  }
                  isEntryInProgress={props.isGenerating && isLastEntry}
                  allowedLinks={[]}
                  isLeoModel={conversationContext.isLeoModel}
                />
              )
            })}

          {!showSteps && <Progress {...props} />}
        </div>
        {taskThumbnail && (
          <div className={styles.taskImage}>
            <img src={taskThumbnail} />
          </div>
        )}
      </div>
    </div>
  )
}

function Progress(props: Props) {
  const conversationContext = useUntrustedConversationContext()
  // Show the most recent completion event
  const lastCompletionEvent = props.assistantEntries
    .flatMap(
      (entry) =>
        entry.events?.filter((event) => !!event && event.completionEvent) ?? [],
    )
    .at(-1)
  // Show important tool use events that require interactivity or for info
  const lastEntryToolUseEvents =
    props.assistantEntries
      .at(-1)
      ?.events?.filter((event) => !!event && !!event.toolUseEvent) ?? []
  // Interactive tools only from most recent entry
  const interactiveToolUseEvents = lastEntryToolUseEvents.filter((event) =>
    interactiveToolNames.includes(event.toolUseEvent?.toolName ?? ''),
  )
  // Most recent (up to 1) important tool use events from any entry
  const importantToolUseEvents = props.assistantEntries
    .flatMap(
      (entry) =>
        entry.events?.filter(
          (event) =>
            !!event
            && !!event.toolUseEvent
            && importantToolNames.includes(event.toolUseEvent?.toolName ?? ''),
        ) ?? [],
    )
    .slice(-1)

  return (
    <div>
      {importantToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event.toolUseEvent!}
          isEntryActive={props.isActiveTask}
        />
      ))}
      {lastCompletionEvent && (
        <AssistantResponse
          events={[lastCompletionEvent]}
          isEntryInteractivityAllowed={false}
          isEntryInProgress={props.isGenerating}
          allowedLinks={[]}
          isLeoModel={conversationContext.isLeoModel}
        />
      )}
      {interactiveToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event.toolUseEvent!}
          isEntryActive={props.isActiveTask}
        />
      ))}
    </div>
  )
}
