/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'

// Types
import * as Mojom from '../../../common/mojom'

// Styles
import styles from './style.module.scss'
import { getLocale } from '$web-common/locale'
import { isFullPageScreenshot } from '../../../common/conversation_history_utils'

/**
 * Formats file size in bytes to human readable format
 * @param bytes - File size in bytes
 * @returns Formatted string (e.g., "1.25 MB")
 */
export const formatFileSize = (bytes: number): string => {
  const units = ['B', 'KB', 'MB', 'GB']
  let index = 0
  let size = bytes

  while (size >= 1024 && index < units.length - 1) {
    size /= 1024
    index++
  }

  return `${size.toFixed(2)} ${units[index]}`
}

type Props = {
  icon: React.ReactNode
  title: string
  subtitle: React.ReactNode

  // remove is optional here so we can also reuse
  // this component in the conversation thread where remove
  // is not needed.
  remove?: () => void
}

const tooltipHideDelay = 0
const tooltipShowDelay = 500

export function AttachmentItem(props: Props) {
  return (
    <div className={styles.itemWrapper}>
      <div className={styles.leftSide}>
        {props.icon}
        <div className={styles.info}>
          <div className={styles.forEllipsis}>
            <span className={styles.title}>{props.title}</span>
          </div>
          {props.subtitle && (
            <span
              data-key='subtitle'
              className={styles.subtitle}
            >
              {props.subtitle}
            </span>
          )}
        </div>
      </div>
      {props.remove && (
        <Button
          fab
          kind='plain-faint'
          className={styles.removeButton}
          onClick={props.remove}
        >
          <Icon name='close' />
        </Button>
      )}
    </div>
  )
}

export function AttachmentSpinnerItem(props: { title: string }) {
  return (
    <AttachmentItem
      icon={
        <div className={styles.loadingContainer}>
          <ProgressRing />
        </div>
      }
      title={props.title}
      subtitle={''}
    />
  )
}

export function AttachmentPageItem(props: {
  title: string
  url: string
  remove?: () => void
}) {
  // We don't display the scheme in the subtitle.
  const sansSchemeUrl = props.url.replace(/^https?:\/\//, '')

  return (
    <AttachmentItem
      icon={
        <div className={styles.favicon}>
          <img
            src={`//favicon2?size=256&pageUrl=${encodeURIComponent(props.url)}&allowGoogleServerFallback=0`}
          />
        </div>
      }
      title={props.title}
      subtitle={
        <>
          {props.remove && (
            <Tooltip
              mode='mini'
              mouseleaveTimeout={tooltipHideDelay}
              mouseenterDelay={tooltipShowDelay}
            >
              <Icon name='info-outline' />
              <div
                className={styles.tooltipContent}
                slot='content'
              >
                {getLocale(S.CHAT_UI_PAGE_ATTACHMENT_TOOLTIP_INFO)}
              </div>
            </Tooltip>
          )}
          <Tooltip
            mode='mini'
            mouseleaveTimeout={tooltipHideDelay}
            mouseenterDelay={tooltipShowDelay}
            className={styles.subtitleText}
          >
            <div>{sansSchemeUrl}</div>
            <div
              className={styles.tooltipContent}
              slot='content'
            >
              {props.url}
            </div>
          </Tooltip>
        </>
      }
      remove={props.remove}
    />
  )
}

function AttachmentUploadItem({
  file,
  index,
  remove,
}: {
  file: Mojom.UploadedFile
  index: number
  remove?: (index: number) => void
}) {
  const isImage =
    file.type === Mojom.UploadedFileType.kImage
    || file.type === Mojom.UploadedFileType.kScreenshot
  const isPdf = file.type === Mojom.UploadedFileType.kPdf
  const isFileFullPageScreenshot = isFullPageScreenshot(file)

  const dataUrl = React.useMemo(() => {
    if (!isImage) return null
    const blob = new Blob([new Uint8Array(file.data)], {
      type: 'image/*',
    })
    return URL.createObjectURL(blob)
  }, [file, isImage])

  const filesize = React.useMemo(() => {
    return formatFileSize(Number(file.filesize))
  }, [file.filesize])

  if (isImage) {
    return (
      <AttachmentItem
        icon={
          <img
            className={styles.image}
            src={dataUrl!}
          />
        }
        title={
          isFileFullPageScreenshot
            ? getLocale(S.CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE)
            : file.filename
        }
        subtitle={filesize}
        remove={remove ? () => remove(index) : undefined}
      />
    )
  } else if (isPdf) {
    return (
      <AttachmentItem
        icon={<Icon name='file' />}
        title={file.filename}
        subtitle={filesize}
        remove={remove ? () => remove(index) : undefined}
      />
    )
  }

  return null
}

export function AttachmentUploadItems(props: {
  uploadedFiles: Mojom.UploadedFile[]
  remove?: (index: number) => void
}) {
  // Calculate first full page screenshot index.
  const firstFullPageScreenshotIndex =
    props.uploadedFiles.findIndex(isFullPageScreenshot)

  return (
    <>
      {props.uploadedFiles
        .filter((file, index) => {
          // Only show the first full page screenshot, hide the rest
          return (
            !isFullPageScreenshot(file)
            || index === firstFullPageScreenshotIndex
          )
        })
        .map((file, filteredIndex) => {
          // Find the original index in the unfiltered array
          const originalIndex = props.uploadedFiles.indexOf(file)

          return (
            <AttachmentUploadItem
              key={`${file.filename}-${originalIndex}`}
              file={file}
              index={originalIndex}
              remove={props.remove}
            />
          )
        })}
    </>
  )
}
