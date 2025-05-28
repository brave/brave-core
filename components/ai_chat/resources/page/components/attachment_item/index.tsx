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

type Props = {
  icon: React.ReactNode
  title: string
  subtitle: string
  // remove is optional here so we can also reuse
  // this component in the conversation thread where remove
  // is not needed.
  remove?: () => void
}

export function AttachmentItem(props: Props) {
  return (
    <div className={styles.itemWrapper}>
      <div className={styles.leftSide}>
        {props.icon}
        <div className={styles.info}>
          <Tooltip
            mode='mini'
            text={props.title}
          >
            <div className={styles.forEllipsis}>
              <span className={styles.title}>{props.title}</span>
            </div>
          </Tooltip>
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

export function AttachmentImageItem(props: {
  remove?: () => void
  uploadedImage: Mojom.UploadedFile
}) {
  const dataUrl = React.useMemo(() => {
    const blob = new Blob([new Uint8Array(props.uploadedImage.data)], {
      type: 'image/*',
    })
    return URL.createObjectURL(blob)
  }, [props.uploadedImage])

  const filesize = React.useMemo(() => {
    let bytes = Number(props.uploadedImage.filesize)
    const units = ['B', 'KB', 'MB', 'GB']
    let index = 0

    while (bytes >= 1024 && index < units.length - 1) {
      bytes /= 1024
      index++
    }

    return `${bytes.toFixed(2)} ${units[index]}`
  }, [props.uploadedImage.filesize])

  return (
    <AttachmentItem
      icon={
        <img
          className={styles.image}
          src={dataUrl}
        />
      }
      title={props.uploadedImage.filename}
      subtitle={filesize}
      remove={props.remove}
    />
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
