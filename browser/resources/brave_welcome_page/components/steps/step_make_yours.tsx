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
import wallpaperLight from '../img/bg-light.jpg'
import wallpaperDark from '../img/bg-dark.jpg'

interface MockBrowserWindowVariantProps {
  themeClass: string
}

function MockBrowserWindowHorizontal({ themeClass }: MockBrowserWindowVariantProps) {
  return (
    <div className={`browser-chrome ${themeClass}`}>
      {/* Tab Bar */}
      <div className="browser-tabbar">
        <div className="browser-tabs">
          {/* Pinned GitHub tab */}
          <div className="browser-tab pinned">
            <Icon name="examplecom-color" />
          </div>
          {/* Active tab */}
          <div className="browser-tab active">
            <Icon name="social-brave-release-favicon-fullheight-color" />
            <span className="tab-title">Welcome to Brave</span>
            <Icon name="close" className="tab-close" />
          </div>
          {/* Inactive tab */}
          <div className="browser-tab">
            <Icon name="brave-icon-search-color" />
            <span className="tab-title">Brave Search</span>
            <div className="tab-divider" />
          </div>
          {/* Add tab button */}
          <div className="browser-tab-add">
            <Icon name="plus-add" />
          </div>
        </div>
        {/* Dropdown button */}
        <div className="browser-tab-dropdown">
          <Icon name="carat-down" />
        </div>
      </div>

      {/* Address Bar */}
      <div className="browser-addressbar">
        <div className="addressbar-actions">
          <div className="toolbar-btn">
            <Icon name="browser-back" />
          </div>
          <div className="toolbar-btn disabled">
            <Icon name="browser-forward" />
          </div>
          <div className="toolbar-btn">
            <Icon name="browser-refresh" />
          </div>
          <div className="toolbar-btn">
            <Icon name="browser-bookmark-normal" />
          </div>
        </div>

        <div className="addressbar-field">
          <div className="toolbar-btn small">
            <Icon name="shield-done" />
          </div>
          <span className="addressbar-url">brave.com</span>
          <div className="toolbar-btn small">
            <Icon name="share-macos" />
          </div>
        </div>

        <div className="addressbar-menu">
          <div className="menu-btn">
            <Icon name="social-brave-release-favicon-fullheight-color" />
            <Icon name="more-vertical" />
          </div>
        </div>
      </div>

      {/* Browser Content */}
      <div className="browser-content">
        <div className="browser-content-bg" />
      </div>
    </div>
  )
}

function MockBrowserWindowVertical({ themeClass }: MockBrowserWindowVariantProps) {
  return (
    <div className={`browser-chrome vertical ${themeClass}`}>
      {/* Address Bar (at top for vertical layout) */}
      <div className="browser-addressbar">
        <div className="addressbar-actions">
          <div className="toolbar-btn">
            <Icon name="browser-back" />
          </div>
          <div className="toolbar-btn disabled">
            <Icon name="browser-forward" />
          </div>
          <div className="toolbar-btn">
            <Icon name="browser-refresh" />
          </div>
          <div className="toolbar-btn">
            <Icon name="browser-bookmark-normal" />
          </div>
        </div>

        <div className="addressbar-field">
          <div className="toolbar-btn small">
            <Icon name="shield-done" />
          </div>
          <span className="addressbar-url">brave.com</span>
          <div className="toolbar-btn small">
            <Icon name="share-macos" />
          </div>
        </div>

        <div className="addressbar-menu">
          <div className="menu-btn">
            <Icon name="social-brave-release-favicon-fullheight-color" />
            <Icon name="more-vertical" />
          </div>
        </div>
      </div>

      {/* Content area with sidebar */}
      <div className="browser-main-content">
        {/* Vertical Tab Sidebar */}
        <div className="vertical-sidebar">
          {/* Pinned tabs row */}
          <div className="pinned-tabs-row">
            <div className="pinned-tab">
              <Icon name="examplecom-color" />
            </div>
            <div className="pinned-tab">
              <Icon name="google-calendar-color" />
            </div>
          </div>

          {/* Divider */}
          <div className="sidebar-divider" />

          {/* Vertical tabs list */}
          <div className="vertical-tabs-list">
            {/* Active tab */}
            <div className="vertical-tab active">
              <Icon name="social-brave-release-favicon-fullheight-color" />
              <span className="vertical-tab-title">Welcome to Brave</span>
              <Icon name="close" className="vertical-tab-close" />
            </div>
            {/* Inactive tab */}
            <div className="vertical-tab">
              <Icon name="brave-icon-search-color" />
              <span className="vertical-tab-title">Brave Search</span>
            </div>
          </div>
        </div>

        {/* Browser Content */}
        <div className="browser-content vertical">
          <div className="browser-content-bg" />
        </div>
      </div>
    </div>
  )
}

interface MockBrowserWindowProps {
  layout: 'horizontal' | 'vertical'
  effectiveTheme: 'light' | 'dark'
}

function MockBrowserWindow({ layout, effectiveTheme }: MockBrowserWindowProps) {
  const themeClass = `preview-theme-${effectiveTheme}`
  
  if (layout === 'vertical') {
    return <MockBrowserWindowVertical themeClass={themeClass} />
  }
  return <MockBrowserWindowHorizontal themeClass={themeClass} />
}

type TabLayout = 'horizontal' | 'vertical'
type Theme = 'system' | 'light' | 'dark'

interface ColorOption {
  id: string
  color1: string  
  color2: string  
  color3: string  
}

const colorOptions: ColorOption[] = [
  { id: 'default', color1: '#FFFFFF', color2: '#D7DBFF', color3: '#4C54D2' },
  { id: 'purple', color1: '#D7DBFF', color2: '#BBC1FF', color3: '#8B8FD6' },
  { id: 'teal', color1: '#CDE3E5', color2: '#AEC3C6', color3: '#5A7A7D' },
  { id: 'cyan', color1: '#B3E6EF', color2: '#77D4E4', color3: '#007A8C' },
  { id: 'green', color1: '#C3E6CD', color2: '#96D5A9', color3: '#2D7A4A' },
  { id: 'olive', color1: '#D5E2CE', color2: '#B6C3B1', color3: '#6B7A66' },
  { id: 'gold', color1: '#FAE29F', color2: '#E2C384', color3: '#8B6914' },
  { id: 'orange', color1: '#F6D7CA', color2: '#FFB597', color3: '#A85232' },
  { id: 'tan', color1: '#FDD6C6', color2: '#DBB8A9', color3: '#8B6E61' },
  { id: 'pink', color1: '#F8D2EA', color2: '#F2B1DB', color3: '#9E4D84' },
  { id: 'dusty-rose', color1: '#FBD5DA', color2: '#D9B6BB', color3: '#8B6A70' },
  { id: 'hot-pink', color1: '#FFD1DF', color2: '#FFB1C9', color3: '#A84D6B' },
  { id: 'lavender', color1: '#E6D7FA', color2: '#D5BBF6', color3: '#7D5A9E' },
]

// Hook to detect system theme preference
function useSystemTheme(): 'light' | 'dark' {
  const [systemTheme, setSystemTheme] = React.useState<'light' | 'dark'>(() => {
    if (typeof window !== 'undefined' && window.matchMedia) {
      return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
    }
    return 'light'
  })

  React.useEffect(() => {
    if (typeof window === 'undefined' || !window.matchMedia) return

    const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')
    const handleChange = (e: MediaQueryListEvent) => {
      setSystemTheme(e.matches ? 'dark' : 'light')
    }

    mediaQuery.addEventListener('change', handleChange)
    return () => mediaQuery.removeEventListener('change', handleChange)
  }, [])

  return systemTheme
}

export function StepMakeYoursContent({}: StepContentProps) {
  const [tabLayout, setTabLayout] = React.useState<TabLayout>('horizontal')
  const [theme, setTheme] = React.useState<Theme>('system')
  const [selectedColor, setSelectedColor] = React.useState<string>('default')
  
  const systemTheme = useSystemTheme()
  
  // Compute effective theme: use system preference when 'system' is selected
  const effectiveTheme: 'light' | 'dark' = theme === 'system' ? systemTheme : theme

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
              <img src={effectiveTheme === 'dark' ? wallpaperDark : wallpaperLight} alt="" className="mock-window-wallpaper-image" />
            </div>
            <MockBrowserWindow layout={tabLayout} effectiveTheme={effectiveTheme} />
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
                    '--swatch-color-1': colorOpt.color1,
                    '--swatch-color-2': colorOpt.color2,
                    '--swatch-color-3': colorOpt.color3,
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

