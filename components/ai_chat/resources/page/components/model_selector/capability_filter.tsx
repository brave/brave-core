// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import {
  ALL_MODEL_CAPABILITIES,
  getModelCapabilityIcon,
  getModelCapabilityLabel,
} from '../../model_utils'
import styles from './style.module.scss'

interface Props {
  selected: Mojom.ModelCapability[]
  onChange: (capabilities: Mojom.ModelCapability[]) => void
  isOpen: boolean
  onOpenChange: (open: boolean) => void
}

export function CapabilityFilter(props: Props) {
  const toggle = (capability: Mojom.ModelCapability) => {
    if (props.selected.includes(capability)) {
      props.onChange(props.selected.filter((c) => c !== capability))
    } else {
      props.onChange([...props.selected, capability])
    }
  }

  return (
    <ButtonMenu
      isOpen={props.isOpen}
      onChange={(e) => props.onOpenChange(e.isOpen)}
      positionStrategy='fixed'
      className={styles.capabilityFilterMenu}
    >
      <Button
        slot='anchor-content'
        kind='plain-faint'
        size='tiny'
        className={classnames({
          [styles.filterButton]: true,
          [styles.filterButtonActive]: props.selected.length > 0,
        })}
        data-testid='capability-filter-button'
      >
        <Icon
          name='filter-settings'
          slot='icon-before'
        />
        {getLocale(S.CHAT_UI_FILTER_MODELS_BUTTON)}
        {props.selected.length > 0 && (
          <Label
            mode='loud'
            color='primary'
            className={styles.filterCount}
          >
            {String(props.selected.length)}
          </Label>
        )}
      </Button>

      {ALL_MODEL_CAPABILITIES.map((capability) => {
        const isSelected = props.selected.includes(capability)
        return (
          <leo-menu-item
            key={capability}
            class={classnames({
              [styles.leoDropdownItem]: true,
              [styles.leoDropdownItemSelected]: isSelected,
            })}
            data-testid={`capability-filter-${capability}`}
            data-is-interactive='true'
            aria-selected={isSelected ? 'true' : null}
            onClick={(e) => {
              e.stopPropagation()
              toggle(capability)
            }}
          >
            <Icon name={getModelCapabilityIcon(capability)} />
            <span className={styles.leoDropdownItemLabel}>
              {getModelCapabilityLabel(capability)}
            </span>
            <Icon
              name='check-circle-filled'
              className={classnames({
                [styles.statusCheck]: true,
                [styles.statusCheckHidden]: !isSelected,
              })}
            />
          </leo-menu-item>
        )
      })}
    </ButtonMenu>
  )
}
