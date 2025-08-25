/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { getString } from '../../lib/strings'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import classNames from '$web-common/classnames'

import {
  SelectedBackgroundType,
  backgroundCSSValue,
  solidBackgrounds,
  gradientBackgrounds } from '../../state/background_state'

import {
  useBackgroundState,
  useCurrentBackground,
  useBackgroundActions } from '../../context/background_context'

interface Props {
  backgroundType: SelectedBackgroundType
  renderUploadOption: () => React.ReactNode
  onClose: () => void
}

export function BackgroundTypePanel(props: Props) {
  const actions = useBackgroundActions()

  const selectedBackground = useBackgroundState((s) => s.selectedBackground)
  const customBackgrounds = useBackgroundState((s) => s.customBackgrounds)
  const currentBackground = useCurrentBackground()

  const type = props.backgroundType

  function panelTitle() {
    switch (type) {
      case SelectedBackgroundType.kCustom:
        return getString(S.NEW_TAB_CUSTOM_BACKGROUND_LABEL)
      case SelectedBackgroundType.kGradient:
        return getString(S.NEW_TAB_GRADIENT_BACKGROUND_LABEL)
      case SelectedBackgroundType.kSolid:
        return getString(S.NEW_TAB_SOLID_BACKGROUND_LABEL)
      default:
        return ''
    }
  }

  function panelValues() {
    switch (type) {
      case SelectedBackgroundType.kCustom: return customBackgrounds
      case SelectedBackgroundType.kGradient: return gradientBackgrounds
      case SelectedBackgroundType.kSolid: return solidBackgrounds
      default: return []
    }
  }

  function onRandomizeToggle(detail: { checked: boolean }) {
    if (detail.checked) {
      actions.selectBackground(type, '')
    } else if (currentBackground) {
      switch (currentBackground.type) {
        case 'custom':
          actions.selectBackground(type, currentBackground.imageUrl)
          break
        case 'color':
          actions.selectBackground(type, currentBackground.cssValue)
          break
        default:
          break
      }
    }
  }

  const values = panelValues()

  return <>
    <h4>
      <button onClick={props.onClose}>
        <Icon name='arrow-left' />
        {panelTitle()}
      </button>
    </h4>
    <div className='control-row'>
      <label>{getString(S.NEW_TAB_RANDOMIZE_BACKGROUND_LABEL)}</label>
      <Toggle
        size='small'
        checked={selectedBackground.type === type && !selectedBackground.value}
        disabled={values.length === 0}
        onChange={onRandomizeToggle}
      />
    </div>
    <div className='background-options'>
      {values.map((value) => {
        const isSelected =
          selectedBackground.type === type &&
          selectedBackground.value === value

        return (
          <div
            key={value}
            className={classNames({
              'background-option': true,
              'can-remove': type === SelectedBackgroundType.kCustom
            })}
          >
            <button onClick={() => { actions.selectBackground(type, value) }}>
              <div
                className='preview'
                style={inlineCSSVars({
                  '--preview-background': backgroundCSSValue(type, value)
                })}
              >
                {
                  isSelected &&
                    <span className='selected-marker'>
                      <Icon name='check-normal' />
                    </span>
                }
              </div>
            </button>
            {
              type === SelectedBackgroundType.kCustom &&
                <button
                  className='remove-image'
                  onClick={() => { actions.removeCustomBackground(value) }}
                >
                  <Icon name='close' />
                </button>
            }
          </div>
        )
      })}
      {type === SelectedBackgroundType.kCustom && props.renderUploadOption()}
    </div>
  </>
}
