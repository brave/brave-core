// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'
import { RailEntry } from '../../model_utils'
import styles from './style.module.scss'

interface Props {
  entries: RailEntry[]
  selectedKey: string
  onSelect: (key: string) => void
}

export function FilterRail(props: Props) {
  return (
    <div
      className={styles.filterRail}
      role='tablist'
      aria-orientation='vertical'
    >
      {props.entries.map((entry) => {
        const isSelected = entry.key === props.selectedKey
        return (
          <Tooltip
            key={entry.key}
            mode='mini'
            text={entry.label}
            placement='top'
            positionStrategy='fixed'
            mouseenterDelay={1000}
          >
            <button
              type='button'
              role='tab'
              aria-selected={isSelected}
              aria-label={entry.label}
              data-testid={`filter-rail-${entry.key}`}
              className={classnames({
                [styles.filterRailButton]: true,
                [styles.filterRailButtonSelected]: isSelected,
              })}
              onClick={() => {
                props.onSelect(entry.key)
              }}
            >
              <Icon
                name={entry.icon}
                className={styles.filterRailIcon}
              />
            </button>
          </Tooltip>
        )
      })}
    </div>
  )
}
