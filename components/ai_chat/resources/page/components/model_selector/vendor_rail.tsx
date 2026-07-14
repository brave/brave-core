// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'
import { getVendorIcon } from '../../../common/vendor_icon_map'
import { VendorRailEntry } from '../../model_utils'
import styles from './style.module.scss'

interface Props {
  entries: VendorRailEntry[]
  selectedKey: string
  onSelect: (key: string) => void
}

export function VendorRail(props: Props) {
  return (
    <div
      className={styles.vendorRail}
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
              data-testid={`vendor-rail-${entry.key}`}
              className={classnames({
                [styles.vendorRailButton]: true,
                [styles.vendorRailButtonSelected]: isSelected,
              })}
              onClick={() => {
                props.onSelect(entry.key)
              }}
            >
              <Icon
                name={getVendorIcon(entry.key)}
                className={styles.vendorRailIcon}
              />
            </button>
          </Tooltip>
        )
      })}
    </div>
  )
}
