// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'
import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'

interface Props {
  value: string
  onChange: (value: string) => void
}

export function ModelSearch(props: Props) {
  return (
    <div className={styles.searchInput}>
      <Input
        type='search'
        size='small'
        placeholder={getLocale(S.CHAT_UI_SEARCH_MODELS_PLACEHOLDER)}
        value={props.value}
        data-testid='model-search-input'
        onInput={(detail) => props.onChange(detail.value)}
      >
        <Icon
          name='search'
          slot='left-icon'
        />
      </Input>
    </div>
  )
}
