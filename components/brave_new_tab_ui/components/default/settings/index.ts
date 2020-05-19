// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'brave-ui/theme'

interface Props {
  textDirection: string
}

export const SettingsMenu = styled<Props, 'div'>('div')`
  width: 680px;
  ${p => p.textDirection && (p.textDirection === 'rtl') ? `left: 12px` : `right: 12px`}
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  border-radius: 8px;
  padding: 24px;
  box-shadow: 0px 4px 24px 0px rgba(0, 0, 0, 0.24);
  font-family: ${p => p.theme.fontFamily.body};
`

export const SettingsContent = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-gap: 40px;
`

export const SettingsSidebar = styled<{}, 'aside'>('aside')`
  position: relative;
  /* normalize against SettingsMenu default padding */
  margin-left: -24px;
  padding-left: 24px;
`

interface SettingsSidebarActiveButtonSliderProps {
  translateTo: number
}

export const SettingsSidebarActiveButtonSlider =
  styled<SettingsSidebarActiveButtonSliderProps, 'div'>('div')`
  position: absolute;
  top: 0;
  left: 0;
  height: 48px;
  width: 4px;
  background: linear-gradient(93.83deg, ${p => p.theme.color.brandBrave} -3.53%, ${p => p.theme.palette.magenta500} 110.11%);
  border-radius: 0px 2px 2px 0px;
  transform: translateY(${p => p.translateTo * 48}px);
  transition-delay: 0.05s;
  transition-duration: 0.3s;
  transition-timing-function: ease-in;
  transition-property: transform;
`
interface SettingsSidebarSVGContentProps {
  src: string
}

export const SettingsSidebarSVGContent = styled<SettingsSidebarSVGContentProps, 'div'>('div')`
  width: 20px;
  height: 20px;
  background: ${p => p.theme.palette.grey800};
  -webkit-mask-image: url(${p => p.src});
  -webkit-mask-repeat: no-repeat;
  -webkit-mask-position: center;
`

export const SettingsSidebarButtonText = styled<{}, 'span'>('span')`
  margin-left: 16px;
  font-weight: 600;
  font-size: 13px;
  font-family: ${p => p.theme.fontFamily.heading};
  color: ${p => p.theme.color.contextMenuForeground};
  position: relative;

  &:hover {
    color: ${p => p.theme.color.brandBrave};
  }
`
interface SettingsSidebarButtonProps {
  activeTab: boolean
}

export const SettingsSidebarButton = styled<SettingsSidebarButtonProps, 'button'>('button')`
  appearance: none;
  padding: 0;
  margin: 0;
  border: 0;
  width: 220px;
  height: 48px;
  text-align: left;
  cursor: pointer;
  display: flex;
  align-items: center;
  background: inherit;

  &:focus {
    outline: none;
  }

  &:hover {
    ${SettingsSidebarSVGContent} {
      background: ${p => p.theme.color.brandBrave};
    }
    ${SettingsSidebarButtonText} {
      color: ${p => p.theme.color.brandBrave};
    }
  }

  ${SettingsSidebarSVGContent} {
    ${p => p.activeTab && css`
      background: linear-gradient(93.83deg, ${p => p.theme.color.brandBrave} -3.53%, ${p => p.theme.palette.magenta500} 110.11%);
    `}
  }

  ${SettingsSidebarButtonText} {
    // Gradientify text
    ${p => p.activeTab && css`
      background: ${p => p.theme.color.panelBackground};
    `}

    ${p => p.activeTab && css`
      background-size: 100%;
      background-repeat: repeat;
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      -moz-background-clip: text;
      -moz-text-fill-color: transparent;
      transition: linear 0.3s background-image;
      background-image: linear-gradient(93.83deg, ${p => p.theme.color.brandBrave} -3.53%, ${p => p.theme.palette.magenta500} 110.11%);
    `}
`

export const SettingsFeatureBody = styled<{}, 'section'>('section')`
  padding: 10px 16px 0;
`

export const SettingsTitle = styled<{}, 'div'>('div')`
  margin-bottom: 17px;
  grid-template-columns: 1fr 20px;
  display: grid;
  align-items: center;

  h1 {
    font-family: ${p => p.theme.fontFamily.heading};
    margin: 0;
    font-weight: 600;
    font-size: 20px;
    line-height: 30px;
    letter-spacing: 0.02em;
    color: ${p => p.theme.color.neutral900};
  }
`

export const SettingsCloseIcon = styled<{}, 'button'>('button')`
  appearance: none;
  padding: 0;
  margin: 0;
  border: 0;
  width: 20px;
  height: 20px;
  cursor: pointer;
  background: inherit;
`

interface SettingsRowProps {
  isChildSetting?: boolean
}

export const SettingsRow = styled<SettingsRowProps, 'div'>('div')`
  box-sizing: border-box;
  display: grid;
  grid-template-columns: 1fr 36px;
  align-items: center;
  width: 100%;
  height: 28px;
  ${p => p.isChildSetting && css`
    padding-left: 36px;
  `}
`

export const SettingsText = styled<{}, 'span'>('span')`
  display: flex;
  align-items: center;
  font-style: normal;
  font-weight: normal;
  font-size: 13px;
  line-height: 24px;
  letter-spacing: 0.01em;
  font-family: ${p => p.theme.fontFamily.heading};
`

interface SettingsWrapperProps {
  textDirection: string
}

export const SettingsWrapper = styled<SettingsWrapperProps, 'div'>('div')`
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 3;
  display: flex;
  justify-content: center;
  align-items: center;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;
  color: rgba(255,255,255,0.8);
  margin-right: ${p => p.textDirection === 'ltr' && '8px'};
  margin-left: ${p => p.textDirection === 'rtl' && '8px'};
  border-right: ${p => p.textDirection === 'ltr' && '1px solid rgba(255, 255, 255, 0.6)'};
  border-left: ${p => p.textDirection === 'rtl' && '1px solid rgba(255, 255, 255, 0.6)'};

  &:hover {
    color: #ffffff;
  }
`
