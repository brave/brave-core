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

import getPageHandlerInstance, * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import styles from './style.module.scss'

function getCategoryName(category: mojom.ModelCategory) {
  // To avoid problems when order of enum values change, we base the key
  // on the enum name rather than the number value, e.g. "CHAT" vs 0
  const categoryKey = Object.keys(mojom.ModelCategory)[category]
  const key = `modelCategory-${categoryKey.toLowerCase()}`
  return getLocale(key)
}

function getIntroMessage(model: mojom.Model) {
  const key = `introMessage-${model.key}`
  return getLocale(key)
}

export default function ModelIntro() {
  const context = React.useContext(DataContext)

  const model = context.currentModel
  if (!model) {
    console.error('Rendered ModelIntro when currentModel does not exist!')
    return <></>
  }

  return (
    <div className={styles.modelInfo}>
      <div className={styles.modelIcon}>
        <Icon name='product-brave-leo' />
      </div>
      <div className={styles.meta}>
        <h4 className={styles.category}>{getCategoryName(model.category)}</h4>
        <h3 className={styles.name}>
          {formatMessage(getLocale('modelNameSyntax'), {
            placeholders: {
              $1: model.displayName,
              $2: model.displayMaker
            }
          })}
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
                          onClick={() => getPageHandlerInstance().pageHandler.openModelSupportUrl()}
                          href="#"
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
        </h3>
      </div>
    </div>
  )
}
