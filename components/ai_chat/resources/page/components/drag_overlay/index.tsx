/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'
import { isImageFile } from '../../constants/file_types'
import { useConversation } from '../../state/conversation_context'
import { convertFileToUploadedFile } from '../../utils/file_utils'

export default function DragOverlay() {
  const {
    isDragActive,
    isDragOver,
    clearDragState,
    attachImages
  } = useConversation()
  const handleOverlayDrop = async (e: React.DragEvent) => {
    e.preventDefault()
    e.stopPropagation()
    clearDragState()

    const files = Array.from(e.dataTransfer?.files || []).filter(isImageFile)

    if (files.length === 0) {
      return
    }

    try {
      const uploadedFiles = await Promise.all(
        files.map(file => convertFileToUploadedFile(file))
      )
      attachImages(uploadedFiles)
    } catch (error) {
      // Silently fail - error will be handled by the upload system
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
            <div className={styles.dragIcon} />
            <div className={styles.dragTitle}>
              {getLocale(S.CHAT_UI_DROP_IMAGES_HERE_LABEL)}
            </div>
            <div className={styles.dragDescription}>
              {getLocale(S.CHAT_UI_DROP_FILES_DESCRIPTION)}
            </div>
          </div>
        </div>
      )}
    </>
  )
}
