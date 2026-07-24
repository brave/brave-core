// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import Button from '@brave/leo/react/button'
import { formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { getModelIcon } from '../../../common/constants'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'

function getIntroMessageKey(model: Mojom.Model) {
  return `CHAT_UI_INTRO_MESSAGE_${model.key.toUpperCase().replaceAll('-', '_')}`
}

interface ModelIntroProps {
  modelKey: string
}

export default function ModelIntro(props: ModelIntroProps) {
  const { modelKey } = props
  const context = useUntrustedConversationContext()
  const state = context.api.useState().data

  // Only needed on iOS to fix one-tap issue with tooltip.
  // <if expr="is_ios">
  const [isTooltipVisible, setIsTooltipVisible] = React.useState(false)
  // </if>

  const model = state.allModels.find((m) => m.key === modelKey)
  if (!model) {
    return <></>
  }

  const isLeoModel = !!model.options.leoModelOptions
  const modelName = isLeoModel
    ? model.displayName
    : model.options.customModelOptions?.modelRequestName

  return (
    <div className={styles.modelInfo}>
      <div className={styles.modelRow}>
        <div
          className={styles.modelIcon}
          data-key={model.key}
        >
          <Icon name={getModelIcon(model)} />
        </div>
        <span className={styles.name}>{modelName}</span>
        {isLeoModel && (
          <Tooltip
            mode='default'
            className={styles.tooltip}
            offset={4}
            // <if expr="is_ios">
            visible={isTooltipVisible}
            // </if>
          >
            <div
              slot='content'
              className={styles.tooltipContent}
            >
              {formatLocale(getIntroMessageKey(model), {
                $1: (content) => {
                  return (
                    <button
                      key={content}
                      onClick={() => context.uiHandler.openModelSupportUrl()}
                    >
                      {content}
                    </button>
                  )
                },
              })}
            </div>
            <Button
              fab
              kind='plain-faint'
              className={styles.tooltipButton}
              // <if expr="is_ios">
              onClick={() => setIsTooltipVisible((prev) => !prev)}
              // </if>
            >
              <Icon name='info-outline' />
            </Button>
          </Tooltip>
        )}
      </div>
    </div>
  )
}
