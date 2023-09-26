// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import styles from './style.module.scss'

function getCategoryName (category: mojom.ModelCategory): string {
  if (category === mojom.ModelCategory.CHAT) {
    return 'Chat' // TODO: translate
  }

  console.error('Cannot provide string for model category: ', category)
  return ''
}

function getIntroMessage (model: mojom.Model) {
  switch (model.key) {
    case 'chat-default':
      return `Hi there! I'm here to help. What can I assist you with today?`
    case 'chat-leo-expanded':
      return `I have a vast base of knowledge and a large memory able to help with more complex challenges`
    case 'chat-claude-instant':
      return `Hi! My name is Claude. I was created by Anthropic to be helpful, harmless, and honest.`
    default:
      console.error(`Did not know intro string for model key: ${model.key}`)
      return `Hi there! I'm here to help. What can I assist you with today?`
  }
}

function ModelDisplayName ({model}: {model: mojom.Model}) {
  // TODO: translate
  return (
    <>
      {model.displayName} by <span className={styles.maker}>{model.displayMaker}</span>
    </>
  )
}

export default function ModelIntro (props: {}) {
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
        <h2 className={styles.category}>
          {getCategoryName(model.category)}
        </h2>
        <h3 className={styles.name}>
          <ModelDisplayName model={model} />
        </h3>
        <p className={styles.modelIntro}>
          {getIntroMessage(model)}
        </p>
      </div>
    </div>
  )
}
