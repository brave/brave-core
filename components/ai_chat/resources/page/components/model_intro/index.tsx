// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import styles from './style.module.scss'

function getCategoryName (category: mojom.ModelCategory) {
  // To avoid problems when order of enum values change, we base the key
  // on the enum name rather than the number value, e.g. "CHAT" vs 0
  const categoryKey = Object.keys(mojom.ModelCategory)[category]
  const key = `modelCategory-${categoryKey.toLowerCase()}`
  return getLocale(key)
}

function getIntroMessage (model: mojom.Model) {
  const key = `introMessage-${model.key}`
  return getLocale(key)
}

export default function ModelIntro () {
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
        </h3>
        <p className={styles.modelIntro}>{getIntroMessage(model)}</p>
      </div>
    </div>
  )
}
