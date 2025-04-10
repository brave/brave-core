/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Types
import * as Mojom from '../../../common/mojom'

// Styles
import styles from './style.module.scss'

type Props = {
  uploadedImage: Mojom.UploadedFile
  // removeImage is optional here so we can also reuse
  // this component in the conversation thread where remove
  // is not needed.
  removeImage?: () => void
}

export default function UploadedImgItem(props: Props) {
  // Memos
  const dataUrl = React.useMemo(() => {
    const blob = new Blob([new Uint8Array(props.uploadedImage.data)], {
      type: 'image/*'
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

  const filenameParts = props.uploadedImage.filename.split('.')

  return (
    <div className={styles.itemWrapper}>
      <div className={styles.leftSide}>
        <img
          className={styles.image}
          src={dataUrl}
        />
        <div className={styles.imageInfo}>
          <div className={styles.nameAndExtensionRow}>
            <div className={styles.forEllipsis}>
              <span className={styles.nameText}>{filenameParts[0]}</span>
            </div>
            <span className={styles.extensionText}>.{filenameParts[1]}</span>
          </div>
          <span className={styles.sizeText}>{filesize}</span>
        </div>
      </div>
      {props.removeImage && (
        <Button
          fab
          kind='plain-faint'
          className={styles.removeButton}
          onClick={props.removeImage}
        >
          <Icon name='close' />
        </Button>
      )}
    </div>
  )
}
