/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { isFullPageScreenshot } from '../../../common/conversation_history_utils'
import { formatFileSize } from '../attachment_item'
import styles from './style.module.scss'

interface Props {
  file: Mojom.UploadedFile | null
  onClose: () => void
}

const MAX_IMAGE_WIDTH = 720
const MAX_IMAGE_HEIGHT = 720

function getImageBlob(file: Mojom.UploadedFile): Blob {
  return new Blob([new Uint8Array(file.data)], { type: 'image/png' })
}

/**
 * Returns the dialog width that fits the image within the viewport caps while
 * preserving aspect ratio. Height is left to the image (`height: auto`) so the
 * preview area always matches the image with no letterboxing.
 */
function getFittedDialogWidth(
  naturalWidth: number,
  naturalHeight: number,
): number {
  const maxWidth = Math.min(window.innerWidth * 0.9, MAX_IMAGE_WIDTH)
  const maxHeight = Math.min(window.innerHeight * 0.7, MAX_IMAGE_HEIGHT)
  let width = Math.min(naturalWidth, maxWidth)
  const heightAtWidth = (width / naturalWidth) * naturalHeight
  if (heightAtWidth > maxHeight) {
    width = (maxHeight / naturalHeight) * naturalWidth
  }
  return Math.max(1, Math.round(width))
}

export default function ImageLightbox(props: Props) {
  const { file, onClose } = props
  const [dialogWidth, setDialogWidth] = React.useState<number | null>(null)
  const [isCopySuccess, setIsCopySuccess] = React.useState(false)
  const copySuccessTimeoutRef = React.useRef<ReturnType<
    typeof setTimeout
  > | null>(null)

  const clearCopySuccessTimeout = React.useCallback(() => {
    if (copySuccessTimeoutRef.current !== null) {
      clearTimeout(copySuccessTimeoutRef.current)
      copySuccessTimeoutRef.current = null
    }
  }, [])

  const dataUrl = React.useMemo(() => {
    if (!file) {
      return null
    }
    return URL.createObjectURL(getImageBlob(file))
  }, [file])

  React.useEffect(() => {
    setDialogWidth(null)
    setIsCopySuccess(false)
    clearCopySuccessTimeout()
  }, [file, clearCopySuccessTimeout])

  React.useEffect(() => {
    return () => {
      if (dataUrl) {
        URL.revokeObjectURL(dataUrl)
      }
    }
  }, [dataUrl])

  React.useEffect(() => {
    return () => {
      clearCopySuccessTimeout()
    }
  }, [clearCopySuccessTimeout])

  const title = React.useMemo(() => {
    if (!file) {
      return ''
    }
    return isFullPageScreenshot(file)
      ? getLocale(S.CHAT_UI_FULL_PAGE_SCREENSHOT_TITLE)
      : file.filename
  }, [file])

  const filesize = React.useMemo(() => {
    if (!file) {
      return ''
    }
    return formatFileSize(Number(file.filesize))
  }, [file])

  const handleImageLoad = React.useCallback(
    (event: React.SyntheticEvent<HTMLImageElement>) => {
      const { naturalWidth, naturalHeight } = event.currentTarget
      if (naturalWidth > 0 && naturalHeight > 0) {
        setDialogWidth(getFittedDialogWidth(naturalWidth, naturalHeight))
      }
    },
    [],
  )

  const showCopySuccess = React.useCallback(() => {
    clearCopySuccessTimeout()
    setIsCopySuccess(true)
    copySuccessTimeoutRef.current = setTimeout(() => {
      setIsCopySuccess(false)
      copySuccessTimeoutRef.current = null
    }, 2000)
  }, [clearCopySuccessTimeout])

  const handleCopy = React.useCallback(async () => {
    if (!file || isCopySuccess) {
      return
    }
    const blob = getImageBlob(file)
    try {
      await navigator.clipboard.write([
        new ClipboardItem({ [blob.type]: blob }),
      ])
      showCopySuccess()
    } catch {
      // Some platforms reject image/png ClipboardItem construction when the
      // blob type is mismatched; retry with an explicit PNG type.
      try {
        await navigator.clipboard.write([
          new ClipboardItem({ 'image/png': blob }),
        ])
        showCopySuccess()
      } catch {
        // Leave the button in its default state if copy fails.
      }
    }
  }, [file, isCopySuccess, showCopySuccess])

  const handleDownload = React.useCallback(() => {
    if (!file || !dataUrl) {
      return
    }
    const link = document.createElement('a')
    link.href = dataUrl
    link.download = file.filename || 'image.png'
    link.click()
  }, [file, dataUrl])

  // Leo Dialog uses --leo-dialog-width as max-width (its own width is
  // nearly full-bleed). Cap max-width to the fitted image width so the
  // dialog shrinks to the image; the <img> uses height: auto so the
  // preview height always matches the aspect ratio with no letterboxing.
  const dialogStyle = dialogWidth
    ? `--leo-dialog-width: ${dialogWidth}px`
    : undefined

  return (
    <Dialog
      isOpen={!!file}
      showClose
      escapeCloses
      backdropClickCloses
      onClose={onClose}
      className={styles.dialog}
      style={dialogStyle}
    >
      {file && (
        <div className={styles.card}>
          <div className={styles.imageContainer}>
            {dataUrl && (
              <img
                className={styles.image}
                src={dataUrl}
                alt={title}
                onLoad={handleImageLoad}
              />
            )}
          </div>
          <div className={styles.footer}>
            <div className={styles.info}>
              <span className={styles.title}>{title}</span>
              <span className={styles.subtitle}>{filesize}</span>
            </div>
            <div className={styles.actions}>
              <Button
                fab
                kind='outline'
                className={
                  isCopySuccess ? styles.copyButtonSuccess : styles.actionButton
                }
                title={getLocale(S.CHAT_UI_IMAGE_LIGHTBOX_COPY_BUTTON_LABEL)}
                aria-label={getLocale(
                  S.CHAT_UI_IMAGE_LIGHTBOX_COPY_BUTTON_LABEL,
                )}
                onClick={handleCopy}
              >
                <Icon name={isCopySuccess ? 'check-normal' : 'copy'} />
              </Button>
              <Button
                fab
                kind='outline'
                className={styles.actionButton}
                title={getLocale(
                  S.CHAT_UI_IMAGE_LIGHTBOX_DOWNLOAD_BUTTON_LABEL,
                )}
                aria-label={getLocale(
                  S.CHAT_UI_IMAGE_LIGHTBOX_DOWNLOAD_BUTTON_LABEL,
                )}
                onClick={handleDownload}
              >
                <Icon name='download' />
              </Button>
            </div>
          </div>
        </div>
      )}
    </Dialog>
  )
}
