// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import Button from '@brave/leo/react/button'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

function getCategoryName(category: Mojom.ModelCategory) {
  // To avoid problems when order of enum values change, we base the key
  // on the enum name rather than the number value, e.g. "CHAT" vs 0
  const categoryKey = Object.keys(Mojom.ModelCategory)[category]
  const key = `modelCategory-${categoryKey.toLowerCase()}`
  return getLocale(key)
}

function getIntroMessage(model: Mojom.Model) {
  const key = `introMessage-${model.key}`
  return getLocale(key)
}

export default function ModelIntro() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const model = conversationContext.currentModel
  if (!model) {
    return <></>
  }

  return (
    <div className={styles.modelInfo}>
      <div className={styles.modelIcon}>
        <Icon name='product-brave-leo' />
      </div>
      <div className={styles.meta}>
        <h4 className={styles.category}>
          {conversationContext.isCurrentModelLeo
            ? getCategoryName(model.options.leoModelOptions!.category)
            : model.displayName}
        </h4>
        <h3 className={styles.name}>
          {conversationContext.isCurrentModelLeo
            ? formatMessage(getLocale('modelNameSyntax'), {
                placeholders: {
                  $1: model.displayName,
                  $2: model.options.leoModelOptions!.displayMaker
                }
              })
            : `${model.options.customModelOptions?.modelRequestName}`}
          {conversationContext.isCurrentModelLeo && (
            <Tooltip
              mode='default'
              className={styles.tooltip}
              offset={4}
            >
              <div
                slot='content'
                className={styles.tooltipContent}
              >
                {formatMessage(getIntroMessage(model), {
                  tags: {
                    $1: (content) => {
                      return (
                        <a
                          key={content}
                          onClick={() =>
                            aiChatContext.uiHandler?.openModelSupportUrl()
                          }
                          href='#'
                          target='_blank'
                        >
                          {content}
                        </a>
                      )
                    }
                  }
                })}
              </div>
              <Button
                fab
                kind='plain-faint'
                className={styles.tooltipButton}
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
