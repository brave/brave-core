/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import { formatLocale, getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

// Asks whether to switch to a vision model before sending images the current
// model can't read.
export default function VisionModelSwitchModal() {
  const conversationContext = useConversation()
  const { currentModel, visionModelSwitchModel } = conversationContext

  if (!visionModelSwitchModel) {
    return null
  }

  return (
    <Dialog
      isOpen
      showClose
      onClose={conversationContext.cancelVisionModelSwitch}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.title}
      >
        {getLocale(S.CHAT_UI_VISION_MODEL_SWITCH_TITLE)}
      </div>
      <div className={styles.body}>
        {formatLocale(S.CHAT_UI_VISION_MODEL_SWITCH_DESCRIPTION, {
          $1: currentModel?.displayName ?? '',
          $2: visionModelSwitchModel.displayName,
        })}
      </div>
      <div
        slot='actions'
        className={styles.actionsRow}
      >
        <Button
          kind='plain-faint'
          size='medium'
          onClick={conversationContext.cancelVisionModelSwitch}
        >
          {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
        </Button>
        <Button
          kind='outline'
          size='medium'
          onClick={() =>
            conversationContext.respondToVisionModelSwitch(
              Mojom.VisionModelSwitchDecision.kSendWithoutImages,
            )
          }
        >
          {getLocale(S.CHAT_UI_VISION_MODEL_SEND_WITHOUT_IMAGES_BUTTON_LABEL)}
        </Button>
        <Button
          kind='filled'
          size='medium'
          onClick={() =>
            conversationContext.respondToVisionModelSwitch(
              Mojom.VisionModelSwitchDecision.kSwitch,
            )
          }
        >
          {getLocale(S.CHAT_UI_VISION_MODEL_SWITCH_BUTTON_LABEL)}
        </Button>
      </div>
    </Dialog>
  )
}
