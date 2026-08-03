/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { getString } from '../../lib/strings'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import classnames from '$web-common/classnames'

import {
  SelectedBackgroundType,
  backgroundCSSValue,
  solidBackgrounds,
  gradientBackgrounds,
} from '../../state/background_store'

import {
  useBackgroundState,
  useCurrentBackground,
  useBackgroundActions,
} from '../../context/background_context'

interface Props {
  backgroundType: SelectedBackgroundType
  renderUploadOption: () => React.ReactNode
}

export function BackgroundTypePanel(props: Props) {
  const actions = useBackgroundActions()

  const selectedBackground = useBackgroundState((s) => s.selectedBackground)
  const customBackgrounds = useBackgroundState((s) => s.customBackgrounds)
  const braveBackgrounds = useBackgroundState((s) => s.braveBackgrounds)
  const disabledBraveBackgrounds = useBackgroundState(
    (s) => s.disabledBraveBackgrounds,
  )
  const currentBackground = useCurrentBackground()

  const type = props.backgroundType
  const isBravePanel = type === SelectedBackgroundType.kBrave

  function panelValues() {
    switch (type) {
      case SelectedBackgroundType.kBrave:
        return braveBackgrounds.map((background) => background.imageUrl)
      case SelectedBackgroundType.kCustom:
        return customBackgrounds
      case SelectedBackgroundType.kGradient:
        return gradientBackgrounds
      case SelectedBackgroundType.kSolid:
        return solidBackgrounds
      default:
        return []
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
  const enabledBraveCount = isBravePanel
    ? values.filter((value) => !disabledBraveBackgrounds.includes(value)).length
    : 0

  return (
    <>
      {!isBravePanel && (
        <div className='control-row'>
          <label>{getString(S.NEW_TAB_RANDOMIZE_BACKGROUND_LABEL)}</label>
          <Toggle
            size='small'
            checked={
              selectedBackground.type === type && !selectedBackground.value
            }
            disabled={values.length === 0}
            onChange={onRandomizeToggle}
          />
        </div>
      )}
      <div className='background-options'>
        {type === SelectedBackgroundType.kCustom && (
          <div className='background-option'>{props.renderUploadOption()}</div>
        )}
        {values.map((value) => {
          const isSelected =
            !isBravePanel
            && selectedBackground.type === type
            && selectedBackground.value === value
          const isDisabled =
            isBravePanel && disabledBraveBackgrounds.includes(value)
          const hasPinnedSelection =
            !isBravePanel
            && selectedBackground.type === type
            && !!selectedBackground.value
          // Dim non-selected tiles when one image is pinned, and always dim
          // disabled Brave backgrounds.
          const isInactive = isDisabled || (hasPinnedSelection && !isSelected)
          const canDisable = enabledBraveCount > 1

          const preview = (
            <div
              className={classnames({
                preview: true,
                inactive: isInactive,
              })}
              style={inlineCSSVars({
                '--preview-background': backgroundCSSValue(type, value),
              })}
            >
              {isSelected && (
                <span className='selected-marker'>
                  <Icon name='check-normal' />
                </span>
              )}
            </div>
          )

          return (
            <div
              key={value}
              className={classnames({
                'background-option': true,
                disabled: isDisabled,
              })}
            >
              {isBravePanel ? (
                preview
              ) : (
                <button
                  onClick={() => {
                    actions.selectBackground(type, value)
                  }}
                >
                  {preview}
                </button>
              )}
              {type === SelectedBackgroundType.kCustom && (
                <button
                  className='overlay-button remove-image'
                  onClick={() => {
                    actions.removeCustomBackground(value)
                  }}
                >
                  <Icon name='close' />
                </button>
              )}
              {isBravePanel && (
                <button
                  className='overlay-button toggle-image'
                  title={getString(
                    isDisabled
                      ? S.NEW_TAB_ENABLE_BACKGROUND_LABEL
                      : S.NEW_TAB_DISABLE_BACKGROUND_LABEL,
                  )}
                  aria-label={getString(
                    isDisabled
                      ? S.NEW_TAB_ENABLE_BACKGROUND_LABEL
                      : S.NEW_TAB_DISABLE_BACKGROUND_LABEL,
                  )}
                  disabled={!isDisabled && !canDisable}
                  onClick={() => {
                    actions.setBraveBackgroundEnabled(value, isDisabled)
                  }}
                >
                  <Icon name={isDisabled ? 'eye-off' : 'eye-on'} />
                </button>
              )}
            </div>
          )
        })}
      </div>
    </>
  )
}
