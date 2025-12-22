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

/**
 * Renders the code execution tool output, displaying the executed JavaScript,
 * console output, and any chart images generated via window.createChart().
 */
const ToolEventCodeExecution: ToolComponent = (props) => {
  const jsCode = props.toolInput?.script

  // Tool output may contain multiple content blocks: text (console.log output)
  // and/or image (rendered chart captured as base64 PNG).
  const outputs = props.toolUseEvent.output ?? []
  const textOutput =
    outputs.find((block) => block.textContentBlock)?.textContentBlock?.text ??
    ''
  const imageOutput = outputs.find(
    (block) => block.imageContentBlock
  )?.imageContentBlock?.imageUrl?.url

  // When toolLabel is null, the parent ToolEvent component renders the
  // expandedContent directly without a collapsible wrapper. This ensures
  // chart images are immediately visible rather than hidden behind a click.
  const content: ToolUseContent = {
    toolLabel: imageOutput ? null : props.content.toolLabel,
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
          {textOutput && (
            <div className={styles.outputSection}>
              <div className={styles.sectionLabel}>
                {getLocale(S.CHAT_UI_CODE_EXECUTION_OUTPUT_LABEL)}
              </div>
              <div className={styles.outputContent}>
                <CodeBlock.Block
                  code={textOutput}
                  lang='text'
                />
              </div>
            </div>
          )}
          {imageOutput && (
            <div className={styles.chartSection}>
              <div className={styles.sectionLabel}>
                {getLocale(S.CHAT_UI_CODE_EXECUTION_CHART_LABEL)}
              </div>
              <div className={styles.chartContent}>
                <img
                  src={imageOutput}
                  alt='Generated chart'
                  className={styles.chartImage}
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
