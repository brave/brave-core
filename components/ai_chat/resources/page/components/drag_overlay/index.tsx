/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'

import '../../../common/strings'

interface DragOverlayProps {
  isDragActive: boolean
  isDragOver: boolean
  onDragOver: (e: React.DragEvent) => void
  onDrop: (e: React.DragEvent) => void
}

export default function DragOverlay({
  isDragActive,
  isDragOver,
  onDragOver,
  onDrop
}: DragOverlayProps) {
  return (
    <>
      {/* Transparent overlay that captures drag events over iframe */}
      {isDragActive && (
        <div
          className={styles.dragDetectionOverlay}
          onDragOver={onDragOver}
          onDrop={onDrop}
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