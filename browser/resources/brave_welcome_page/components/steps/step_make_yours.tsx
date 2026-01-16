/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'
import Tooltip from '@brave/leo/react/tooltip'

import { StepContentProps, StepFooterProps } from '../types'
import wallpaperLight from '../img/bg-light.jpg'
import wallpaperDark from '../img/bg-dark.jpg'

type TabLayout = 'horizontal' | 'vertical'
type Theme = 'system' | 'light' | 'dark'

// Default values for customization options
const DEFAULT_TAB_LAYOUT: TabLayout = 'horizontal'
const DEFAULT_THEME: Theme = 'system'
const DEFAULT_COLOR: string = 'Default'

// Context to share customization state between content and footer
interface MakeYoursContextType {
  hasChanges: boolean
  setHasChanges: (value: boolean) => void
}

const MakeYoursContext = React.createContext<MakeYoursContextType>({
  hasChanges: false,
  setHasChanges: () => {}
})

export function MakeYoursProvider({ children }: { children: React.ReactNode }) {
  const [hasChanges, setHasChanges] = React.useState(false)
  
  return (
    <MakeYoursContext.Provider value={{ hasChanges, setHasChanges }}>
      {children}
    </MakeYoursContext.Provider>
  )
}

interface MockBrowserWindowVariantProps {
  themeClass: string
  colorStyles: React.CSSProperties
}

function MockBrowserWindowHorizontal({ themeClass, colorStyles }: MockBrowserWindowVariantProps) {
  return (
    <div className={`browser-chrome ${themeClass}`} style={colorStyles}>
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

function MockBrowserWindowVertical({ themeClass, colorStyles }: MockBrowserWindowVariantProps) {
  return (
    <div className={`browser-chrome vertical ${themeClass}`} style={colorStyles}>
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
  colorOption: ColorOption
}

function MockBrowserWindow({ layout, effectiveTheme, colorOption }: MockBrowserWindowProps) {
  const themeClass = `preview-theme-${effectiveTheme}`
  const colorStyles = generateThemedColors(colorOption, effectiveTheme === 'dark')
  
  if (layout === 'vertical') {
    return <MockBrowserWindowVertical themeClass={themeClass} colorStyles={colorStyles} />
  }
  return <MockBrowserWindowHorizontal themeClass={themeClass} colorStyles={colorStyles} />
}

interface ColorOption {
  id: string
  color1: string  
  color2: string  
  color3: string  
}

// Helper to convert hex to HSL
function hexToHSL(hex: string): { h: number; s: number; l: number } {
  const r = parseInt(hex.slice(1, 3), 16) / 255
  const g = parseInt(hex.slice(3, 5), 16) / 255
  const b = parseInt(hex.slice(5, 7), 16) / 255

  const max = Math.max(r, g, b)
  const min = Math.min(r, g, b)
  let h = 0
  let s = 0
  const l = (max + min) / 2

  if (max !== min) {
    const d = max - min
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min)
    switch (max) {
      case r: h = ((g - b) / d + (g < b ? 6 : 0)) / 6; break
      case g: h = ((b - r) / d + 2) / 6; break
      case b: h = ((r - g) / d + 4) / 6; break
    }
  }

  return { h: h * 360, s: s * 100, l: l * 100 }
}

// Helper to convert HSL to hex
function hslToHex(h: number, s: number, l: number): string {
  s /= 100
  l /= 100
  const a = s * Math.min(l, 1 - l)
  const f = (n: number) => {
    const k = (n + h / 30) % 12
    const color = l - a * Math.max(Math.min(k - 3, 9 - k, 1), -1)
    return Math.round(255 * color).toString(16).padStart(2, '0')
  }
  return `#${f(0)}${f(8)}${f(4)}`
}

// ============================================
// THEME CONFIGURATION - Tweak these values!
// ============================================

// Saturation multiplier (0-1): higher = more colorful
const LIGHT_MODE_SATURATION = 0.9  // Light mode saturation
const DARK_MODE_SATURATION = 0.4   // Dark mode saturation

// Light mode luminosity: higher values = lighter (0-100)
const LIGHT_MODE_LUMINOSITY = {
  base: 90,           // Base luminosity for light mode
  bgOffset: 4,        // bg = base + offset (brighter)
  highlightOffset: -2, // highlight = base + offset
  tabbarOffset: -4,   // tabbar = base + offset (darker)
}

// Dark mode luminosity: lower values = darker (0-100)
const DARK_MODE_LUMINOSITY = {
  base: 13,           // Base luminosity for dark mode
  bgOffset: 0,        // bg = base + offset (reference)
  highlightOffset: 6, // highlight = base + offset (lighter)
  tabbarOffset: -4,   // tabbar = base + offset (darker)
}
// ============================================

// Generate themed colors from a color option
function generateThemedColors(colorOpt: ColorOption, isDark: boolean): React.CSSProperties {
  // For default, use neutral colors (no tinting)
  if (colorOpt.id === 'Default') {
    return {}
  }

  // Get the hue from the accent color (color3)
  const hsl = hexToHSL(colorOpt.color3)
  const hue = hsl.h
  // Use similar saturation to the original color but capped for subtlety
  const maxSat = Math.min(hsl.s, 25)

  if (isDark) {
    const { base, bgOffset, highlightOffset, tabbarOffset } = DARK_MODE_LUMINOSITY
    const sat = maxSat * DARK_MODE_SATURATION
    return {
      '--preview-bg': hslToHex(hue, sat, base + bgOffset),
      '--preview-bg-highlight': hslToHex(hue, sat * 1.2, base + highlightOffset),
      '--preview-tabbar-bg': hslToHex(hue, sat * 0.8, base + tabbarOffset),
    } as React.CSSProperties
  } else {
    const { base, bgOffset, highlightOffset, tabbarOffset } = LIGHT_MODE_LUMINOSITY
    const sat = maxSat * LIGHT_MODE_SATURATION
    return {
      '--preview-bg': hslToHex(hue, sat, base + bgOffset),
      '--preview-bg-highlight': hslToHex(hue, sat * 1.2, base + highlightOffset),
      '--preview-tabbar-bg': hslToHex(hue, sat, base + tabbarOffset),
    } as React.CSSProperties
  }
}

const colorOptions: ColorOption[] = [
  { id: 'Default', color1: '#E3E3E3', color2: '#C7C7C7', color3: '#4C54D2' },
  { id: 'Blue', color1: '#D4E3FF', color2: '#A4C8FF', color3: '#2D5F9D' },
  { id: 'Cool grey', color1: '#D4E3FF', color2: '#B8C7E7', color3: '#505F79' },
  { id: 'Grey', color1: '#D1E8EB', color2: '#B5CCCF', color3: '#4D6366' },
  { id: 'Aqua', color1: '#6CF7E7', color2: '#48DACB', color3: '#006D62' },
  { id: 'Green', color1: '#ABF4A1', color2: '#90D788', color3: '#296A27' },
  { id: 'Viridian', color1: '#D7E6D1', color2: '#BBCAB7', color3: '#546252' },
  { id: 'Citron', color1: '#FFE169', color2: '#E3C550', color3: '#735C00' },
  { id: 'Orange', color1: '#FFDAC4', color2: '#FFB37F', color3: '#994A07' },
  { id: 'Apricot', color1: '#FFDBCA', color2: '#E4C0AF', color3: '#75594B' },
  { id: 'Rose', color1: '#FFD8DF', color2: '#FFAEC0', color3: '#9E4159' },
  { id: 'Pink', color1: '#FFDADF', color2: '#E3BEC3', color3: '#75575C' },
  { id: 'Fuchsia', color1: '#FFD6F7', color2: '#FFACEE', color3: '#8C4680' },
  { id: 'Lavender', color1: '#EFDCFF', color2: '#D9BAFF', color3: '#6F5198' },
]

// Format color ID to display name (e.g., 'dusty-rose' → 'Dusty Rose')
function formatColorName(id: string): string {
  return id
    .split('-')
    .map(word => word.charAt(0).toUpperCase() + word.slice(1))
    .join(' ')
}

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
  const [tabLayout, setTabLayout] = React.useState<TabLayout>(DEFAULT_TAB_LAYOUT)
  const [theme, setTheme] = React.useState<Theme>(DEFAULT_THEME)
  const [selectedColor, setSelectedColor] = React.useState<string>(DEFAULT_COLOR)
  
  const { setHasChanges } = React.useContext(MakeYoursContext)
  const systemTheme = useSystemTheme()
  
  // Track whether any changes have been made from defaults
  React.useEffect(() => {
    const hasAnyChanges = 
      tabLayout !== DEFAULT_TAB_LAYOUT ||
      theme !== DEFAULT_THEME ||
      selectedColor !== DEFAULT_COLOR
    setHasChanges(hasAnyChanges)
  }, [tabLayout, theme, selectedColor, setHasChanges])
  
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
              <img 
                src={wallpaperLight} 
                alt="" 
                className={`mock-window-wallpaper-image ${effectiveTheme === 'light' ? 'wallpaper-visible' : 'wallpaper-hidden'}`} 
              />
              <img 
                src={wallpaperDark} 
                alt="" 
                className={`mock-window-wallpaper-image ${effectiveTheme === 'dark' ? 'wallpaper-visible' : 'wallpaper-hidden'}`} 
              />
            </div>
            <MockBrowserWindow 
              layout={tabLayout} 
              effectiveTheme={effectiveTheme} 
              colorOption={colorOptions.find(c => c.id === selectedColor) || colorOptions[0]}
            />
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
                <Tooltip key={colorOpt.id} mode='mini' text={formatColorName(colorOpt.id)} mouseenterDelay={500}>
                  <button
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
                </Tooltip>
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
  const { hasChanges } = React.useContext(MakeYoursContext)
  
  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="plain-faint" size="large" onClick={onSkip}>Use defaults</Button>
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          {hasChanges ? 'Apply and continue' : 'Continue'}
        </Button>
      </div>
    </div>
  )
}

