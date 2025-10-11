// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import { getLocale } from '$web-common/locale'
import CodeBlock from '../code_block'
import { ToolComponent } from './tool_event'
import styles from './code_execution_tool_event.module.scss'
import '../../../common/strings'

const CodeExecutionToolEvent: ToolComponent = ({
  toolUseEvent,
  toolInput,
  content,
  children,
}) => {
  // Extract the code from the tool input
  const jsCode = React.useMemo(() => {
    if (!toolInput?.script) return ''
    return typeof toolInput.script === 'string' ? toolInput.script : ''
  }, [toolInput])

  // Extract the output from the tool use event
  const output = React.useMemo(() => {
    if (!toolUseEvent.output?.[0]?.textContentBlock?.text) return ''
    return toolUseEvent.output[0].textContentBlock.text
  }, [toolUseEvent.output])

  // Custom content for the code execution tool
  const customContent = {
    ...content,
    toolText: (
      <div className={styles.codeExecutionTool}>
        <span>{getLocale(S.CHAT_UI_CODE_EXECUTION_TITLE)}</span>
        {jsCode && (
          <div className={styles.codeExecutionContent}>
            <div className={styles.codeSection}>
              <div className={styles.sectionLabel}>
                {getLocale(S.CHAT_UI_CODE_EXECUTION_CODE_LABEL)}
              </div>
              <CodeBlock.Block
                code={jsCode}
                lang='javascript'
              />
            </div>
            {output && (
              <div className={styles.outputSection}>
                <div className={styles.sectionLabel}>
                  {getLocale(S.CHAT_UI_CODE_EXECUTION_OUTPUT_LABEL)}
                </div>
                <div className={styles.outputContent}>
                  <CodeBlock.Block
                    code={output}
                    lang='text'
                  />
                </div>
              </div>
            )}
          </div>
        )}
      </div>
    ),
    statusIcon: (
      <span data-testid='code-execution-completed-icon'>
        <Icon name='code' />
      </span>
    ),
    progressIcon: (
      <span data-testid='code-execution-progress-icon'>
        <ProgressRing />
      </span>
    ),
  }

  return children(customContent)
}

export default CodeExecutionToolEvent
