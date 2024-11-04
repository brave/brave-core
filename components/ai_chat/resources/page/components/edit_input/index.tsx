// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'

import styles from './style.module.scss'

interface Props {
  text: string
  onSubmit: (text: string) => void
  onCancel: () => void
  isSubmitDisabled: boolean
}

function EditInput(props: Props) {
  const [text, setText] = React.useState(props.text)
  const textareaRef = React.useCallback((node: HTMLTextAreaElement | null) => {
    /**
     * When editing, we want the cursor to be positioned
     * at the end of the text. Typically we would also call
     * focus() here, but that isn't necessary in this case
     * because the textarea already has [autoFocus] set.
     */
    node && node.setSelectionRange(node.value.length, node.value.length)
  }, [])

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey && !e.nativeEvent.isComposing &&
        !props.isSubmitDisabled) {
      if (!e.repeat) {
        props.onSubmit(e.currentTarget.value)
      }
      e.preventDefault()
    } else if (e.key === 'Escape') {
      props.onCancel()
    }
  }

  return (
    <div className={styles.content}>
      <div
        className={styles.growWrap}
        data-replicated-value={text}
      >
          <textarea
            ref={textareaRef}
            value={text}
            onChange={(e) => setText(e.currentTarget.value)}
            onKeyDown={handleKeyDown}
            autoFocus
            rows={1}
          />
      </div>
      <div className={styles.actions}>
        <Button
          size='small'
          kind='plain-faint'
          onClick={props.onCancel}
        >
        <span className={styles.buttonText}>
          {getLocale('cancelButtonLabel')}
        </span>
        </Button>
        <Button
          size='small'
          kind='filled'
          isDisabled={props.isSubmitDisabled}
          onClick={() => props.onSubmit(text)}
        >
        {getLocale('saveButtonLabel')}
        </Button>
      </div>
    </div>
  )
}

export default EditInput
