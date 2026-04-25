// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import CodeBlock from '../code_block'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event_code_execution.module.scss'
import '../../../common/strings'

const ToolEventCodeExecution: ToolComponent = (props) => {
  const jsCode = props.toolInput?.script
  const output = props.toolUseEvent.output?.[0]?.textContentBlock?.text ?? ''

  const content: ToolUseContent = {
    toolLabel: props.content.toolLabel,
    expandedContent: jsCode && (
      <div className={styles.codeExecutionTool}>
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
      </div>
    ),
  }

  return props.children(content)
}

export default ToolEventCodeExecution
