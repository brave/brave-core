/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'

import { StepContentProps, StepFooterProps } from '../types'
import wallpaperImage from '../img/bg-light.jpg'

type TabLayout = 'horizontal' | 'vertical'
type Theme = 'system' | 'light' | 'dark'

interface ColorOption {
  id: string
  lightTop: string
  lightBottom: string
  darkTop: string
  darkBottom: string
}

const colorOptions: ColorOption[] = [
  { id: 'default', lightTop: '#FFFFFF', lightBottom: '#E4E4E5', darkTop: '#39457A', darkBottom: '#39457A' },
  { id: 'purple', lightTop: '#D7DBFF', lightBottom: '#BBC1FF', darkTop: '#414379', darkBottom: '#414379' },
  { id: 'teal', lightTop: '#CDE3E5', lightBottom: '#AEC3C6', darkTop: '#465759', darkBottom: '#465759' },
  { id: 'cyan', lightTop: '#B3E6EF', lightBottom: '#77D4E4', darkTop: '#005261', darkBottom: '#005261' },
  { id: 'green', lightTop: '#C3E6CD', lightBottom: '#96D5A9', darkTop: '#165330', darkBottom: '#165330' },
  { id: 'olive', lightTop: '#D5E2CE', lightBottom: '#B6C3B1', darkTop: '#4C574A', darkBottom: '#4C574A' },
  { id: 'gold', lightTop: '#FAE29F', lightBottom: '#E2C384', darkTop: '#5E4507', darkBottom: '#5E4507' },
  { id: 'orange', lightTop: '#F6D7CA', lightBottom: '#FFB597', darkTop: '#76381F', darkBottom: '#76381F' },
  { id: 'tan', lightTop: '#FDD6C6', lightBottom: '#DBB8A9', darkTop: '#664F45', darkBottom: '#664F45' },
  { id: 'pink', lightTop: '#F8D2EA', lightBottom: '#F2B1DB', darkTop: '#6C355B', darkBottom: '#6C355B' },
  { id: 'dusty-rose', lightTop: '#FBD5DA', lightBottom: '#D9B6BB', darkTop: '#664D51', darkBottom: '#664D51' },
  { id: 'hot-pink', lightTop: '#FFD1DF', lightBottom: '#FFB1C9', darkTop: '#75334B', darkBottom: '#75334B' },
  { id: 'lavender', lightTop: '#E6D7FA', lightBottom: '#D5BBF6', darkTop: '#563D72', darkBottom: '#563D72' },
]

export function StepMakeYoursContent({}: StepContentProps) {
  const [tabLayout, setTabLayout] = React.useState<TabLayout>('horizontal')
  const [theme, setTheme] = React.useState<Theme>('system')
  const [selectedColor, setSelectedColor] = React.useState<string>('default')

  const handleTabLayoutChange = (detail: { value: string | undefined }) => {
    if (detail.value) {
      setTabLayout(detail.value as TabLayout)
    }
  }

  const handleThemeChange = (detail: { value: string | undefined }) => {
    if (detail.value) {
      setTheme(detail.value as Theme)
    }
  }

  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Make Brave yours</h1>
          <p>Personalize your browsing experience with your preferred tab layout and color modes.</p>
        </div>
      </div>
      <div className="right-content">
        <div className="customize-content">
          {/* Mock Window Preview */}
          <div className="mock-window-preview">
            <div className="mock-window-wallpaper">
              <img src={wallpaperImage} alt="" className="mock-window-wallpaper-image" />
            </div>
            <Icon name="social-brave-release-favicon-fullheight-color" className="mock-window-brave-icon" />
          </div>

          {/* Options */}
          <div className="customize-options">
          {/* Tab Layout Option */}
          <div className="customize-option-row">
            <span className="customize-option-label">Tab layout</span>
            <SegmentedControl value={tabLayout} onChange={handleTabLayoutChange}>
              <SegmentedControlItem value="horizontal">
                <Icon name="window-tabs-horizontal" slot="icon-before" />
                Horizontal
              </SegmentedControlItem>
              <SegmentedControlItem value="vertical">
                <Icon name="window-tabs-vertical-expanded" slot="icon-before" />
                Vertical
              </SegmentedControlItem>
            </SegmentedControl>
          </div>

          {/* Appearance Card */}
          <div className="customize-appearance-card">
            {/* Theme Row */}
            <div className="customize-option-row">
              <span className="customize-option-label">Theme</span>
              <SegmentedControl value={theme} onChange={handleThemeChange}>
                <SegmentedControlItem value="system">
                  <Icon name="theme-system" slot="icon-before" />
                  Device
                </SegmentedControlItem>
                <SegmentedControlItem value="light">
                  <Icon name="theme-light" slot="icon-before" />
                  Light
                </SegmentedControlItem>
                <SegmentedControlItem value="dark">
                  <Icon name="theme-dark" slot="icon-before" />
                  Dark
                </SegmentedControlItem>
              </SegmentedControl>
            </div>

            {/* Divider */}
            <div className="customize-divider" />

            {/* Color Swatches */}
            <div className="customize-colors-row">
              {colorOptions.map((colorOpt) => (
                <button
                  key={colorOpt.id}
                  className={`color-swatch ${selectedColor === colorOpt.id ? 'color-swatch-selected' : ''}`}
                  onClick={() => setSelectedColor(colorOpt.id)}
                  aria-label={`Select ${colorOpt.id} color`}
                  style={{
                    '--swatch-light-top': colorOpt.lightTop,
                    '--swatch-light-bottom': colorOpt.lightBottom,
                  } as React.CSSProperties}
                >
                  {selectedColor === colorOpt.id && (
                    <Icon name="check-circle-filled" className="color-swatch-check" />
                  )}
                </button>
              ))}
            </div>
          </div>
          </div>
        </div>
      </div>
    </div>
  )
}

export function StepMakeYoursFooter({ onNext, onBack, onSkip }: StepFooterProps) {
  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="plain-faint" size="large" onClick={onSkip}>Skip</Button>
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          Continue
        </Button>
      </div>
    </div>
  )
}

