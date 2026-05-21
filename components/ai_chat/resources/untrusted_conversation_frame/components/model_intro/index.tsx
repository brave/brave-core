// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import Button from '@brave/leo/react/button'
import { getLocale, formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'
import { getKeysForMojomEnum } from '$web-common/mojomUtils'

function getCategoryName(category: Mojom.ModelCategory) {
  // To avoid problems when order of enum values change, we base the key
  // on the enum name rather than the number value, e.g. "CHAT" vs 0
  const categoryKey = getKeysForMojomEnum(Mojom.ModelCategory)[category]
  return getLocale('CHAT_UI_MODEL_CATEGORY_' + categoryKey)
}

function getIntroMessageKey(model: Mojom.Model) {
  return `CHAT_UI_INTRO_MESSAGE_${model.key.toUpperCase().replaceAll('-', '_')}`
}

export default function ModelIntro() {
  const context = useUntrustedConversationContext()
  const state = context.api.useState().data

  // Only needed on iOS to fix one-tap issue with tooltip.
  // <if expr="is_ios">
  const [isTooltipVisible, setIsTooltipVisible] = React.useState(false)
  // </if>

  const model = state.allModels.find((m) => m.key === state.currentModelKey)
  if (!model) {
    return <></>
  }

  const isLeoModel = state.isLeoModel

  return (
    <div className={styles.modelInfo}>
      <div className={styles.modelIcon}>
        <Icon name='product-brave-leo' />
      </div>
      <div className={styles.meta}>
        <h4 className={styles.category}>
          {isLeoModel
            ? getCategoryName(model.options.leoModelOptions!.category)
            : model.displayName}
        </h4>
        <h3 className={styles.name}>
          {isLeoModel
            ? model.displayName
            : model.options.customModelOptions?.modelRequestName}
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
        </h3>
      </div>
    </div>
  )
}
