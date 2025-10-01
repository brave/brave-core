// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tabs from '@brave/leo/react/tabs'
import TabItem from '@brave/leo/react/tabItem'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import API from '../../untrusted_conversation_frame_api'
import AssistantResponse from '../assistant_response'
import ToolEvent from '../assistant_response/tool_event'
import styles from './assistant_task.module.scss'
import useExtractTaskData, { TaskData } from './use_extract_task_data'

interface Props {
  // Entries that make up the task loop
  assistantEntries: Mojom.ConversationTurn[]
  isActiveTask: boolean
  isGenerating: boolean
  isLeoModel: boolean
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
      API.getInstance().uiObserver.removeListener(id)
    }
  }, [props.isActiveTask, conversationContext.contentTaskTabId])

  const taskData = useExtractTaskData(props.assistantEntries)

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
          {showSteps && <Steps {...props} taskData={taskData} />}

          {!showSteps && <Progress {...props} taskData={taskData} />}
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

function Progress(props: Props & { taskData: TaskData }) {
  // The Progress tab should show:
  // - Any last active complete "important" tool (TODO, navigate)
  // - the most recent completion event
  // - any tool use events from the most recent entry in the group (the active
  // events)
  const lastTaskItem = props.taskData.taskItems.at(-1)

  if (!lastTaskItem || lastTaskItem.length === 0) {
    return null
  }

  const currentCompletionEvent = lastTaskItem.find(event => !!event.completionEvent)

  // Include any current tool use events that are not already included in the
  // "important" section.
  const currentToolUseEvents = lastTaskItem.filter(
    (event) => !!event.toolUseEvent && !props.taskData.importantToolUseEvents.includes(event.toolUseEvent),
  )

  return (
    <div>
      {props.taskData.importantToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event}
          isEntryActive={props.isActiveTask}
        />
      ))}
      {currentCompletionEvent && (
        <AssistantResponse
          events={[currentCompletionEvent]}
          isEntryInteractivityAllowed={false}
          isEntryInProgress={props.isGenerating}
          allowedLinks={[]}
          isLeoModel={props.isLeoModel}
        />
      )}
      {currentToolUseEvents.map((event, index) => (
        <ToolEvent
          key={index}
          toolUseEvent={event.toolUseEvent!}
          isEntryActive={props.isActiveTask}
        />
      ))}
    </div>
  )
}

function Steps(props: Props & { taskData: TaskData }) {
  // Render every event in the task, split by completion event
  // so that the LLM tells a story of the task by it's own progress
  // description.
  return props.taskData.taskItems.map((taskItem, index) => {
    const isActive = index === props.taskData.taskItems.length - 1
    return (
      <div
        key={index}
        className={classnames(styles.taskStep, !isActive && styles.isComplete)}
      >
        <div className={styles.taskIcon}>
          {isActive ? <ProgressRing /> : <Icon name='check-circle-outline' />}
        </div>
        <AssistantResponse
          events={taskItem}
          isEntryInteractivityAllowed={isActive}
          isEntryInProgress={props.isGenerating && isActive}
          allowedLinks={props.taskData.allowedLinks}
          isLeoModel={props.isLeoModel}
        />
      </div>
    )
  })
}
