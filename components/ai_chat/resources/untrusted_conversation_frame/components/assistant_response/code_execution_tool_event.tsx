// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import CodeBlock from '../code_block'
import { ToolComponent } from './tool_event'
import styles from './code_execution_tool_event.module.scss'

const CodeExecutionToolEvent: ToolComponent = ({
  toolUseEvent,
  toolInput,
  content,
  children,
}) => {
  // Extract the JavaScript code from the tool input
  const jsCode = React.useMemo(() => {
    if (!toolInput?.program) return ''
    return typeof toolInput.program === 'string' ? toolInput.program : ''
  }, [toolInput])

  // Extract the output from the tool use event
  const output = React.useMemo(() => {
    if (!toolUseEvent.output?.[0]?.textContentBlock?.text) return ''
    return toolUseEvent.output[0].textContentBlock.text
  }, [toolUseEvent.output])

  const isComplete = !!toolUseEvent.output

  // Custom content for the code execution tool
  const customContent = {
    ...content,
    toolText: (
      <div className={styles.codeExecutionTool}>
        <span>Code Execution</span>
        {jsCode && (
          <div className={styles.codeExecutionContent}>
            <div className={styles.codeSection}>
              <div className={styles.sectionLabel}>JavaScript Code:</div>
              <CodeBlock.Block
                code={jsCode}
                lang='javascript'
              />
            </div>
            {isComplete && output && (
              <div className={styles.outputSection}>
                <div className={styles.sectionLabel}>Output:</div>
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
