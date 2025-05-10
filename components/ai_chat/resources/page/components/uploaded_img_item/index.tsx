/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'
import { getLocale } from '$web-common/locale'

// Types
import * as Mojom from '../../../common/mojom'

// Styles
import styles from './style.module.scss'

type Props = {
  uploadedImage?: Mojom.UploadedFile
  // removeImage is optional here so we can also reuse
  // this component in the conversation thread where remove
  // is not needed.
  removeImage?: () => void
  cancelFileUpload?: () => void
  isLoading?: boolean
}

export default function UploadedImgItem(props: Props) {
  // Memos
  const dataUrl = React.useMemo(() => {
    if (!props?.uploadedImage?.data) {
      return ''
    }
    const blob = new Blob([new Uint8Array(props.uploadedImage.data)], {
      type: 'image/*'
    })
    return URL.createObjectURL(blob)
  }, [props?.uploadedImage?.data])

  const filesize = React.useMemo(() => {
    if (!props?.uploadedImage?.filesize) {
      return ''
    }
    let bytes = Number(props.uploadedImage.filesize)
    const units = ['B', 'KB', 'MB', 'GB']
    let index = 0

    while (bytes >= 1024 && index < units.length - 1) {
      bytes /= 1024
      index++
    }

    return `${bytes.toFixed(2)} ${units[index]}`
  }, [props?.uploadedImage?.filesize])

  return (
    <div className={styles.itemWrapper}>
      {props.isLoading && (
        <div className={styles.leftSide}>
          <div className={styles.loadingContainer}>
            <ProgressRing />
          </div>
          <span className={styles.uploadingText}>
            {getLocale('uploadingFileLabel')}
          </span>
        </div>
      )}
      {!props.isLoading && (
        <div className={styles.leftSide}>
          <img
            className={styles.image}
            src={dataUrl}
          />
          <div className={styles.imageInfo}>
            <Tooltip
              mode='mini'
              text={props?.uploadedImage?.filename ?? ''}
            >
              <div className={styles.forEllipsis}>
                <span className={styles.nameText}>
                  {props?.uploadedImage?.filename ?? ''}
                </span>
              </div>
            </Tooltip>
            <span className={styles.sizeText}>{filesize}</span>
          </div>
        </div>
      )}
      {props.removeImage && (
        <Button
          fab
          kind='plain-faint'
          className={styles.removeAndCancelButton}
          onClick={props.removeImage}
        >
          <Icon name='close' />
        </Button>
      )}
      {props.isLoading && props.cancelFileUpload && (
        <Button
          kind='plain-faint'
          className={styles.removeAndCancelButton}
          onClick={props.cancelFileUpload}
        >
          {getLocale('cancelButtonLabel')}
        </Button>
      )}
    </div>
  )
}
