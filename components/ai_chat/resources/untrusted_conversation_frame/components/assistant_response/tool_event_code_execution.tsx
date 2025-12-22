// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import CodeBlock from '../code_block'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event_code_execution.module.scss'
import '../../../common/strings'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'

/**
 * Renders the code execution tool output, displaying the executed JavaScript,
 * console output, chart images, and PDF files generated.
 *
 * PDF Generation Flow:
 * 1. Backend (code_execution_tool.cc) executes user script in sandboxed WebContents
 * 2. Script calls window.createPdf() which uses pdf-lib to generate PDF bytes
 * 3. PDF bytes are converted to base64 data URL and returned in ExecutionResult
 * 4. Backend sends PDF as ImageContentBlock with "data:application/pdf;base64,..." URL
 * 5. This component detects the application/pdf MIME type and renders download button
 * 6. On click, downloadDataUrl() sends request to trusted parent frame via Mojo
 * 7. Parent frame (conversation_context.tsx) creates anchor element and triggers download
 */
const ToolEventCodeExecution: ToolComponent = (props) => {
  const context = useUntrustedConversationContext()
  const jsCode = props.toolInput?.script

  // Tool output may contain multiple content blocks: text (console.log output),
  // image (rendered chart as base64 PNG), or PDF (as base64 data URL).
  const outputs = props.toolUseEvent.output ?? []
  const textOutput =
    outputs.find((block) => block.textContentBlock)?.textContentBlock?.text ??
    ''

  // Find image outputs - separate chart images from PDF data URLs.
  // Both charts and PDFs come through as ImageContentBlock, but with different
  // MIME types in the data URL. Charts use "data:image/png;base64,..." while
  // PDFs use "data:application/pdf;base64,...".
  const imageBlocks = outputs.filter((block) => block.imageContentBlock)
  const chartOutput = imageBlocks.find(
    (block) =>
      block.imageContentBlock?.imageUrl?.url?.startsWith('data:image/')
  )?.imageContentBlock?.imageUrl?.url
  const pdfOutput = imageBlocks.find(
    (block) =>
      block.imageContentBlock?.imageUrl?.url?.startsWith('data:application/pdf')
  )?.imageContentBlock?.imageUrl?.url

  const handleDownloadPdf = () => {
    if (!pdfOutput) return

    // Use the parent frame to trigger the download since the untrusted frame
    // is sandboxed and cannot trigger downloads directly.
    context.parentUiFrame?.downloadDataUrl(pdfOutput, 'generated.pdf')
  }

  // When toolLabel is null, the parent ToolEvent component renders the
  // expandedContent directly without a collapsible wrapper. This ensures
  // chart images and PDFs are immediately visible rather than hidden behind a click.
  const hasVisualOutput = chartOutput || pdfOutput
  const content: ToolUseContent = {
    toolLabel: hasVisualOutput ? null : props.content.toolLabel,
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
          {chartOutput && (
            <div className={styles.chartSection}>
              <div className={styles.sectionLabel}>
                {getLocale(S.CHAT_UI_CODE_EXECUTION_CHART_LABEL)}
              </div>
              <div className={styles.chartContent}>
                <img
                  src={chartOutput}
                  alt='Generated chart'
                  className={styles.chartImage}
                />
              </div>
            </div>
          )}
          {pdfOutput && (
            <div className={styles.pdfSection}>
              <div className={styles.sectionLabel}>
                {getLocale(S.CHAT_UI_CODE_EXECUTION_PDF_LABEL)}
              </div>
              <div className={styles.pdfContent}>
                <Button
                  kind='outline'
                  size='small'
                  onClick={handleDownloadPdf}
                >
                  <div slot='icon-before'>
                    <Icon name='download' />
                  </div>
                  <div>{getLocale(S.CHAT_UI_CODE_EXECUTION_DOWNLOAD_PDF_BUTTON)}</div>
                </Button>
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
