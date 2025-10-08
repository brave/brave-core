/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Toggle from '@brave/leo/react/toggle'

import { useBackgroundState, useBackgroundActions } from '../../context/background_context'
import { useRewardsState } from '../../context/rewards_context'
import { getString } from '../../lib/strings'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { BackgroundTypePanel } from './background_type_panel'
import { Link } from '../common/link'
import formatMessage from '$web-common/formatMessage'
import { settingsURL } from '../../../../../components/brave_rewards/resources/shared/lib/rewards_urls'

import {
  SelectedBackgroundType,
  backgroundCSSValue,
  gradientPreviewBackground,
  solidPreviewBackground } from '../../state/background_state'

import { style } from './background_panel.style'

export function BackgroundPanel() {
  const actions = useBackgroundActions()

  const backgroundsEnabled = useBackgroundState((s) => s.backgroundsEnabled)
  const backgroundsCustomizable =
    useBackgroundState((s) => s.backgroundsCustomizable)
  const sponsoredImagesEnabled =
    useBackgroundState((s) => s.sponsoredImagesEnabled)
  const selectedBackground = useBackgroundState((s) => s.selectedBackground)
  const braveBackgrounds = useBackgroundState((s) => s.braveBackgrounds)
  const customBackgrounds = useBackgroundState((s) => s.customBackgrounds)
  const rewardsFeatureEnabled = useRewardsState((s) => s.rewardsFeatureEnabled)
  const rewardsEnabled = useRewardsState((s) => s.rewardsEnabled)

  const [panelType, setPanelType] =
    React.useState<SelectedBackgroundType | null>(null)
  const [uploading, setUploading] = React.useState(false)

  React.useEffect(() => {
    setUploading(false)
  }, [selectedBackground, customBackgrounds])

  function setPanel(type?: SelectedBackgroundType) {
    setPanelType(type ?? null)
  }

  function getTypePreviewValue(type: SelectedBackgroundType) {
    const isSelectedType = type === selectedBackground.type
    switch (type) {
      case SelectedBackgroundType.kBrave:
        return braveBackgrounds[0]?.imageUrl ?? ''
      case SelectedBackgroundType.kCustom:
        if (isSelectedType && selectedBackground.value) {
          return selectedBackground.value
        }
        return customBackgrounds[0] ?? ''
      case SelectedBackgroundType.kSolid:
        if (isSelectedType && selectedBackground.value) {
          return selectedBackground.value
        }
        return solidPreviewBackground
      case SelectedBackgroundType.kGradient:
        if (isSelectedType && selectedBackground.value) {
          return selectedBackground.value
        }
        return gradientPreviewBackground
      default:
        console.error('Unhandled background type', type)
        return ''
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

  function renderTypePreview(type: SelectedBackgroundType) {
    if (type === SelectedBackgroundType.kCustom &&
        customBackgrounds.length === 0) {
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
          type === selectedBackground.type &&
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
      setPanel(SelectedBackgroundType.kCustom)
    }
  }

  if (panelType !== null) {
    return (
      <div data-css-scope={style.scope}>
        <BackgroundTypePanel
          backgroundType={panelType}
          renderUploadOption={() => (
            <button onClick={showCustomBackgroundChooser}>
              {renderUploadPreview()}
            </button>
          )}
          onClose={() => setPanel()}
        />
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString('showBackgroundsLabel')}</label>
        <Toggle
          size='small'
          checked={backgroundsEnabled}
          onChange={({ checked }) => { actions.setBackgroundsEnabled(checked) }}
        />
      </div>
      {
        backgroundsEnabled && rewardsFeatureEnabled &&
          <div className='control-row'>
            <label>
              {getString('showSponsoredImagesLabel')}
              <div className='subtext'>
                {
                  !rewardsEnabled && formatMessage(
                    getString('showSponsoredImagesEarningText'),
                    {
                      tags: {
                        $1: (content) => (
                          <Link
                            key='learn-more'
                            url={settingsURL}
                            openInNewTab
                          >
                            {content}
                          </Link>
                        )
                      }
                    })
                }
              </div>
            </label>
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
                {renderTypePreview(SelectedBackgroundType.kCustom)}
                {getString('customBackgroundLabel')}
              </button>
            </div>
            <div className='background-option'>
              <button
                onClick={() => {
                  actions.selectBackground(SelectedBackgroundType.kBrave, '')
                }}
              >
                {renderTypePreview(SelectedBackgroundType.kBrave)}
                {getString('braveBackgroundLabel')}
              </button>
            </div>
            <div className='background-option'>
              <button onClick={() => setPanel(SelectedBackgroundType.kSolid)}>
                {renderTypePreview(SelectedBackgroundType.kSolid)}
                {getString('solidBackgroundLabel')}
              </button>
            </div>
            <div className='background-option'>
              <button
                onClick={() => setPanel(SelectedBackgroundType.kGradient)}
              >
                {renderTypePreview(SelectedBackgroundType.kGradient)}
                {getString('gradientBackgroundLabel')}
              </button>
            </div>
          </div>
        </>
      }
    </div>
  )
}
