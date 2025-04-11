/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import { AIChatContext } from '../../state/ai_chat_context'
import styles from './style.module.scss'
import {
  IGNORE_EXTERNAL_LINK_WARNING_KEY //
} from '../../../common/constants'
import { ConversationContext } from '../../state/conversation_context'

type Props = Pick<
  ConversationContext,
  'generatedUrlToBeOpened' | 'setGeneratedUrlToBeOpened'
> &
  Pick<AIChatContext, 'uiHandler'>

interface OpenExternalLinkModalProps {
  context: Props
}

export default function OpenExternalLinkModal(
  props: OpenExternalLinkModalProps
) {
  const { context } = props

  // State
  const [ignoreChecked, setIgnoreChecked] = React.useState(false)

  // Methods

  const onOpenClicked = React.useCallback(() => {
    if (ignoreChecked) {
      localStorage.setItem(IGNORE_EXTERNAL_LINK_WARNING_KEY, 'true')
    }
    if (context.generatedUrlToBeOpened) {
      context.uiHandler?.openURL(context.generatedUrlToBeOpened)
    }
    context.setGeneratedUrlToBeOpened(undefined)
  }, [ignoreChecked, context.generatedUrlToBeOpened, context.uiHandler])

  return (
    <Dialog
      isOpen={!!context.generatedUrlToBeOpened}
      showClose
      onClose={() => context.setGeneratedUrlToBeOpened(undefined)}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.dialogTitle}
      >
        {getLocale('openExternalLink')}
      </div>
      <div className={styles.description}>
        {getLocale('openExternalLinkInfo')}
        <Checkbox
          checked={ignoreChecked}
          onChange={({ checked }) => setIgnoreChecked(checked)}
        >
          <span>{getLocale('openExternalLinkCheckboxLabel')}</span>
        </Checkbox>
      </div>
      <div
        slot='actions'
        className={styles.actionsRow}
      >
        <div className={styles.buttonsWrapper}>
          <Button
            kind='plain-faint'
            size='medium'
            onClick={() => context.setGeneratedUrlToBeOpened(undefined)}
          >
            {getLocale('cancelButtonLabel')}
          </Button>
          <Button
            kind='filled'
            size='medium'
            onClick={onOpenClicked}
          >
            {getLocale('openLabel')}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}
