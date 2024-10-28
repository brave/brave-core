/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Toggle from '@brave/leo/react/toggle'

import { useAppActions, useAppState } from '../context/app_model_context'
import { useLocale } from '../context/locale_context'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { BackgroundTypePanel } from './background_type_panel'

import {
  BackgroundType,
  backgroundCSSValue,
  gradientPreviewBackground,
  solidPreviewBackground } from '../../models/backgrounds'

import { style } from './background_panel.style'

export function BackgroundPanel() {
  const { getString } = useLocale()
  const actions = useAppActions()

  const backgroundsEnabled = useAppState((s) => s.backgroundsEnabled)
  const backgroundsCustomizable = useAppState((s) => s.backgroundsCustomizable)
  const sponsoredImagesEnabled = useAppState((s) => s.sponsoredImagesEnabled)
  const selectedBackgroundType = useAppState((s) => s.selectedBackgroundType)
  const selectedBackground = useAppState((s) => s.selectedBackground)
  const braveBackgrounds = useAppState((s) => s.braveBackgrounds)
  const customBackgrounds = useAppState((s) => s.customBackgrounds)

  const [panelType, setPanelType] = React.useState<BackgroundType | null>(null)
  const [uploading, setUploading] = React.useState(false)

  React.useEffect(() => {
    setUploading(false)
  }, [selectedBackground, customBackgrounds])

  function getTypePreviewValue(type: BackgroundType) {
    const isSelectedType = type === selectedBackgroundType
    switch (type) {
      case 'brave':
        return braveBackgrounds[0]?.imageUrl ?? ''
      case 'custom':
        if (isSelectedType && selectedBackground) {
          return selectedBackground
        }
        return customBackgrounds[0] ?? ''
      case 'solid':
        if (isSelectedType && selectedBackground) {
          return selectedBackground
        }
        return solidPreviewBackground
      case 'gradient':
        if (isSelectedType && selectedBackground) {
          return selectedBackground
        }
        return gradientPreviewBackground
    }
  }

  function renderUploadPreview() {
    return (
      <div className='preview upload'>
        {uploading ? <ProgressRing /> : <Icon name='upload' />}
        {getString('uploadBackgroundLabel')}
      </div>
    )
  }

  function renderTypePreview(type: BackgroundType) {
    if (type === 'custom' && customBackgrounds.length === 0) {
      return renderUploadPreview()
    }
    return (
      <div
        className='preview'
        style={inlineCSSVars({
          '--preview-background':
            backgroundCSSValue(type, getTypePreviewValue(type))
        })}
      >
        {
          type === selectedBackgroundType &&
            <span className='selected-marker'>
              <Icon name='check-normal' />
            </span>
        }
      </div>
    )
  }

  function showCustomBackgroundChooser() {
    actions.showCustomBackgroundChooser().then((backgroundSelected) => {
      if (backgroundSelected) {
        setUploading(true)
      }
    })
  }

  function onCustomPreviewClick() {
    if (customBackgrounds.length === 0) {
      showCustomBackgroundChooser()
    } else {
      setPanelType('custom')
    }
  }

  if (panelType) {
    return (
      <div data-css-scope={style.scope}>
        <BackgroundTypePanel
          backgroundType={panelType}
          renderUploadOption={() => (
            <button onClick={showCustomBackgroundChooser}>
              {renderUploadPreview()}
            </button>
          )}
          onClose={() => { setPanelType(null) }}
        />
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='toggle-row'>
        <label>{getString('showBackgroundsLabel')}</label>
        <Toggle
          size='small'
          checked={backgroundsEnabled}
          onChange={({ checked }) => { actions.setBackgroundsEnabled(checked) }}
        />
      </div>
      {
        backgroundsEnabled &&
          <div className='toggle-row'>
            <label>{getString('showSponsoredImagesLabel')}</label>
            <Toggle
              size='small'
              checked={sponsoredImagesEnabled}
              onChange={({ checked }) => {
                actions.setSponsoredImagesEnabled(checked)
              }}
            />
          </div>
      }
      {
        backgroundsEnabled && backgroundsCustomizable && <>
          <div className='background-options'>
            <div className='background-option'>
              <button onClick={onCustomPreviewClick}>
                {renderTypePreview('custom')}
                {getString('customBackgroundLabel')}
              </button>
            </div>
            <div className='background-option'>
              <button onClick={() => actions.selectBackground('brave', '')}>
                {renderTypePreview('brave')}
                {getString('braveBackgroundLabel')}
              </button>
            </div>
            <div className='background-option'>
              <button onClick={() => setPanelType('solid')}>
                {renderTypePreview('solid')}
                {getString('solidBackgroundLabel')}
              </button>
            </div>
            <div className='background-option'>
              <button onClick={() => setPanelType('gradient')}>
                {renderTypePreview('gradient')}
                {getString('gradientBackgroundLabel')}
              </button>
            </div>
          </div>
        </>
      }
    </div>
  )
}
