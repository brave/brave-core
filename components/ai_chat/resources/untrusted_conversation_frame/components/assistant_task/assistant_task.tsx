// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import ProgressRing from '@brave/leo/react/progressRing'
import Tabs from '@brave/leo/react/tabs'
import TabItem from '@brave/leo/react/tabItem'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantResponse from '../assistant_response'
import ToolEvent, { ToolEventThinking } from '../assistant_response/tool_event'
import styles from './assistant_task.module.scss'
import useExtractTaskData, { TaskData } from './use_extract_task_data'

interface Props {
  // Entries that make up the task loop
  assistantEntries: Mojom.ConversationTurn[]

  // Whether the task can be interacted with (usually the most recent entry in
  // the conversation). Informs whether interactivity is allowed.
  isActiveTask: boolean

  // Passes to AssistantResponse
  isLeoModel: boolean
}

interface TabProps {
  // Whether the task is actively still being generated. Informs whether the UI
  // displays as if it's still in progress.
  isGenerating: boolean

  // Whether the most recent tool use request is being executed
  isToolExecuting: boolean

  // Whether the task is currently generating a response
  isThinking: boolean

  // What the state of the active task is
  toolUseTaskState: Mojom.TaskState

  taskData: TaskData
}

/**
 * The Task is split *not* by ConversationTurn but by CompletionEvent, which is
 * better for descriptive UI.
 * The Progress tab displays the most recent item in that split (the
 * completion text and the subsequent tool uses) as well as any previous
 * "important" tool use (e.g. TODO list or tab navigation).
 * The Steps tab displays everything in timeline order.
 */
export default function AssistantTask(props: Props) {
  const [showSteps, setShowSteps] = React.useState(false)
  const [taskThumbnail, setTaskThumbnail] = React.useState<string>()
  const conversationContext = useUntrustedConversationContext()

  React.useEffect(() => {
    // We only currently support a single task per conversation - only show or
    // update a thumbnail if this task is active otherwise it will seem like
    // an older task is making changes to a tab.
    // TODO(https://github.com/brave/brave-browser/issues/49258): support a
    // tab-per-ToolUseEvent and keep track of which tool uses are for which tab
    // when multi-tab agent conversations are supported.
    if (!conversationContext.contentTaskTabId || !props.isActiveTask) {
      return
    }

    // Task is active task - if we get thumbnails for a related Tab, display
    // it.
    const id = conversationContext.uiObserver?.thumbnailUpdated.addListener(
      (tabId: number, dataURI: string) => {
        if (tabId === conversationContext.contentTaskTabId) {
          setTaskThumbnail(dataURI)
        }
      },
    )

    // Let the thumbnail tracker know we want to track the thumbnail of
    // the active task's tab.
    conversationContext.uiHandler?.addTabToThumbnailTracker(
      conversationContext.contentTaskTabId,
    )

    // Stop listening for thumbnails when we stop being the active task.
    return () => {
      conversationContext.uiObserver?.removeListener(id)
      conversationContext.uiHandler?.removeTabFromThumbnailTracker(
        conversationContext.contentTaskTabId!,
      )
    }
  }, [props.isActiveTask, conversationContext.contentTaskTabId])

  const taskData = useExtractTaskData(props.assistantEntries)

  const isThinking =
    props.isActiveTask
    && !conversationContext.isToolExecuting
    && conversationContext.toolUseTaskState === Mojom.TaskState.kRunning

  const tabProps: TabProps = {
    isGenerating: conversationContext.isGenerating,
    isToolExecuting: conversationContext.isToolExecuting,
    toolUseTaskState: conversationContext.toolUseTaskState,
    isThinking: isThinking,
    taskData: taskData,
  }

  return (
    <div
      className={styles.task}
      data-testid='assistant-task'
    >
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
          {showSteps && (
            <Steps
              {...props}
              {...tabProps}
            />
          )}

          {!showSteps && (
            <Progress
              {...props}
              {...tabProps}
            />
          )}

          {props.isActiveTask
            && conversationContext.toolUseTaskState
              === Mojom.TaskState.kPaused && (
              <Label
                color='neutral'
                mode='loud'
                className={styles.taskStateLabel}
              >
                <span data-testid='assistant-task-paused-label'>
                  {getLocale(S.CHAT_UI_TASK_STATE_PAUSED_LABEL)}
                </span>
              </Label>
            )}

          {props.isActiveTask
            && conversationContext.toolUseTaskState
              === Mojom.TaskState.kStopped && (
              <Label
                color='neutral'
                mode='loud'
                className={styles.taskStateLabel}
              >
                <span data-testid='assistant-task-stopped-label'>
                  {getLocale(S.CHAT_UI_TASK_STATE_STOPPED_LABEL)}
                </span>
              </Label>
            )}
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

function Progress(props: Props & TabProps) {
  // The Progress tab should show:
  // - Any last active complete "important" tool (TODO, navigate)
  // - the most recent completion event
  // - any tool use events from the most recent entry in the group (the active
  // events)
  const lastTaskItem = props.taskData.taskItems.at(-1)

  if (!lastTaskItem || lastTaskItem.length === 0) {
    return null
  }

  // If lastTaskItem has a completion event, it will be the first element.
  // If it doesn't then .completionEvent will be undefined.
  const currentCompletionEvent = lastTaskItem[0].completionEvent
    ? lastTaskItem[0]
    : undefined

  // Include any current tool use events that are not already included in the
  // "important" section.
  const currentToolUseEvents = lastTaskItem.filter(
    (event) =>
      event.toolUseEvent
      && !props.taskData.importantToolUseEvents.includes(event.toolUseEvent),
  )

  return (
    <div className={styles.progress}>
      {props.taskData.importantToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event}
          isEntryActive={false}
          isExecuting={false}
        />
      ))}
      {currentCompletionEvent && (
        <div className={styles.progressText}>
          <AssistantResponse
            events={[currentCompletionEvent]}
            isEntryInteractivityAllowed={false}
            isEntryInProgress={props.isGenerating}
            allowedLinks={[]}
            isLeoModel={props.isLeoModel}
          />
        </div>
      )}
      {currentToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event.toolUseEvent!}
          isEntryActive={props.isActiveTask}
          isExecuting={props.isToolExecuting}
        />
      ))}
      {props.isThinking && <ToolEventThinking />}
    </div>
  )
}

function Steps(props: Props & TabProps) {
  // Render every event in the task, split by completion event
  // so that the LLM tells a story of the task by it's own progress
  // description.
  return props.taskData.taskItems.map((taskItem, index) => {
    // Can we interact or run any pending tools?
    const isRunnable =
      props.isActiveTask && index === props.taskData.taskItems.length - 1

    // Are we generating this task item?
    const isActive = isRunnable && props.isToolExecuting && props.isActiveTask

    // Are we waiting for this task item to be completed (shouldn't show as complete)
    const isPending =
      isRunnable
      && !isActive
      && taskItem.some(
        (event) => event.toolUseEvent && !event.toolUseEvent.output,
      )

    const isThinking = isRunnable && props.isThinking

    return (
      <div
        key={index}
        className={classnames(
          styles.taskStep,
          !isActive && styles.isComplete,
          isPending && styles.isPending,
        )}
      >
        <div className={styles.taskIcon}>
          {isActive ? (
            <ProgressRing />
          ) : (
            <Icon name={isPending ? 'help-outline' : 'check-circle-outline'} />
          )}
        </div>
        <AssistantResponse
          events={taskItem}
          isEntryInteractivityAllowed={isRunnable}
          isEntryInProgress={isActive}
          allowedLinks={props.taskData.allowedLinks}
          isLeoModel={props.isLeoModel}
        />
        {isThinking && <ToolEventThinking />}
      </div>
    )
  })
}
