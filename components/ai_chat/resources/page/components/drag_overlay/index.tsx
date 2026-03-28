/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import styles from './style.module.scss'
import { isImageFile, isPdfFile } from '../../constants/file_types'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { convertFileToUploadedFile } from '../../utils/file_utils'

export default function DragOverlay() {
  const aiChat = useAIChat()
  const { isDragActive, isDragOver, clearDragState, attachImages } =
    useConversation()
  const handleOverlayDrop = async (e: React.DragEvent) => {
    e.preventDefault()
    e.stopPropagation()
    clearDragState()

    const files = Array.from(e.dataTransfer?.files || []).filter(
      (file) => isImageFile(file) || isPdfFile(file),
    )

    if (files.length > 0) {
      try {
        const uploadedFiles = await Promise.all(
          files.map((file) =>
            convertFileToUploadedFile(file, aiChat.processImageFile),
          ),
        )
        attachImages(uploadedFiles)
      } catch (error) {
        // Silently fail - error will be handled by the upload system
      }
      return
    }

    // Web image drag: no File objects, but may carry a URL via text/uri-list.
    // Confirm it's an image by checking for an <img tag in text/html, then
    // fetch and process the URL via C++ to bypass CORS restrictions.
    const html = e.dataTransfer?.getData('text/html') ?? ''
    const uriList = e.dataTransfer?.getData('text/uri-list') ?? ''
    const url = uriList.split('\n').map((u) => u.trim()).find(
      (u) => u && !u.startsWith('#'),
    )

    if (html.includes('<img') && url) {
      try {
        const result = await aiChat.fetchAndProcessImageUrl([{ url }])
        if (result) {
          attachImages([result])
        }
      } catch (error) {
        // Silently fail - not all web image drags will be fetchable
      }
    }
  }

  const handleOverlayDragOver = (e: React.DragEvent) => {
    e.preventDefault()
  }
  return (
    <>
      {/* Transparent overlay that captures drag events over iframe */}
      {isDragActive && (
        <div
          className={styles.dragDetectionOverlay}
          onDragOver={handleOverlayDragOver}
          onDrop={handleOverlayDrop}
        />
      )}

      {isDragOver && (
        <div className={styles.dragOverlay}>
          <div className={styles.dragOverlayContent}>
            <div className={styles.dragOverlayContentInner}>
              <div className={styles.dragIcon}>
                <Icon name='file-upload' />
              </div>
              <div className={styles.dragTitle}>
                {getLocale(S.CHAT_UI_DROP_IMAGES_HERE_LABEL)}
              </div>
              <div className={styles.dragDescription}>
                {getLocale(S.CHAT_UI_DROP_FILES_DESCRIPTION)}
              </div>
            </div>
          </div>
        </div>
      )}
    </>
  )
}
