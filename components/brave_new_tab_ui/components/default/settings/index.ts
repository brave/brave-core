// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'brave-ui/theme'

interface Props {
  textDirection: string
}

const isDarkTheme = (p: any) => {
  return p.theme.name === 'Brave Dark'
}

export const SettingsMenu = styled<Props, 'div'>('div')`
  width: 720px;
  min-width: 720px;
  ${p => p.textDirection && (p.textDirection === 'rtl') ? `left: 12px` : `right: 12px`}
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  border-radius: 8px;
  padding: 24px;
  padding-bottom: 0px;
  box-shadow: 0px 4px 24px 0px rgba(0, 0, 0, 0.24);
  font-family: ${p => p.theme.fontFamily.body};
`

export const SettingsContent = styled<{}, 'div'>('div')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-gap: 20px;

  @media screen and (max-width: 1150px) {
    grid-gap: 0px;
  }
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
  src: string,
  isActive: boolean
}

export const SettingsSidebarSVGContent = styled<SettingsSidebarSVGContentProps, 'div'>('div')`
  position: relative;
  width: 20px;
  height: 20px;
  background: ${p => isDarkTheme(p) ? p.theme.palette.grey400 : p.theme.palette.grey800};
  -webkit-mask-image: url(${p => p.src});
  -webkit-mask-repeat: no-repeat;
  -webkit-mask-position: center;
  flex-shrink: 0;

  transition: background var(--sidebar-button-transition-timing) ease-in-out;

  ${p => p.isActive && css`
    --active-opacity: 1;
  `}

  /* Active version (hidden until item is active).
    This is a separate element so that we can:
    1. fade it in (no transition for background gradient) */
  &:after {
    position: absolute;
    display: block;
    content: '';
    opacity: var(--active-opacity, 0);
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(93.83deg, ${p => p.theme.color.brandBrave} -3.53%, ${p => p.theme.palette.magenta500} 110.11%);
    -webkit-mask-image: url(${p => p.src});
    -webkit-mask-repeat: no-repeat;
    -webkit-mask-position: center;
    transition: opacity var(--sidebar-button-transition-timing) ease-in-out;
  }

`

export const SettingsSidebarButtonText = styled<{ isActive: boolean }, 'span'>('span')`
  margin-left: 16px;
  font-weight: 500;
  font-size: 13px;
  font-family: ${p => p.theme.fontFamily.heading};
  line-height: normal;
  color: ${p => p.theme.color.contextMenuForeground};
  position: relative;
  overflow: hidden;
  text-overflow: ellipsis;
  text-transform: capitalize;

  transition: opacity var(--sidebar-button-transition-timing) ease-in-out,
              color var(--sidebar-button-transition-timing) ease-in-out;

  &:hover {
    color: ${p => p.theme.color.brandBrave};
  }

  ${p => p.isActive && css`
    --active-opacity: 1;
    font-weight: 600;
    color: ${p => p.theme.palette.magenta500};
  `}

  /* Active version (hidden until item is active).
     This is a separate element so that we can:
     1. fade it in (no transition for background gradient)
     2. still show ellipsis for overflowing text (which doesn't show for
     background-clip: text) */
  &:after {
    content: attr(data-text);
    position: absolute;
    opacity: var(--active-opacity, 0);
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: ${p => p.theme.color.panelBackground};
    background-size: 100%;
    background-repeat: repeat;
    background-clip: text;
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-image: linear-gradient(93.83deg, ${p => p.theme.color.brandBrave} -3.53%, ${p => p.theme.palette.magenta500} 110.11%);
    overflow: hidden;
    text-overflow: ellipsis;
    transition: opacity var(--sidebar-button-transition-timing) ease-in-out,
                font-weight var(--sidebar-button-transition-timing) ease-in-out;
  }
`

interface SettingsSidebarButtonProps {
  activeTab: boolean
}

export const SettingsSidebarButton = styled<SettingsSidebarButtonProps, 'button'>('button')`
  --sidebar-button-transition-timing: .12s;
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

  &:hover {
    ${SettingsSidebarSVGContent} {
      background: ${p => p.theme.color.brandBrave};
    }
    ${SettingsSidebarButtonText} {
      color: ${p => p.theme.color.brandBrave};
    }
  }

  &:active {
    outline: none;
  }

  &:focus {
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
  }
`

export const SettingsFeatureBody = styled<{}, 'section'>('section')`
  padding: 10px 0;
  height: 360px;
  overflow-y: auto;
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
  grid-template-columns: auto max-content;
  align-items: center;
  width: 100%;
  height: 28px;
  ${p => p.isChildSetting && css`
    padding-left: 36px;
  `}

  /* TODO(petemill): Use specific Component for Content and Control sides */
  :last-child
  {
    justify-self: end;
  }
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
  background: rgba(33, 37, 41, 0.32);

  &:hover {
    color: #ffffff;
  }
`

export const SettingsWidget = styled<{}, 'div'>('div')`
  float: left;
  width: 48%;
  margin-top: 20px;
`

export const StyledWidgetSettings = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.heading};

  ${SettingsWidget}:nth-child(even) {
    margin-right: 17px;
  }
`

export const FeaturedSettingsWidget = styled<{}, 'div'>('div')`
  width: 100%;
`

export const StyledBannerImage = styled<{}, 'img'>('img')`
  width: 100%;
  margin-bottom: 10px;
`

export const StyledSettingsInfo = styled<{}, 'div'>('div')`
  float: left;
  max-width: 250px;
`

export const StyledSettingsTitle = styled<{}, 'div'>('div')`
  font-weight: 600;
  font-size: 14px;
  margin-bottom: 5px;
`

export const StyledSettingsCopy = styled<{}, 'div'>('div')`
  font-size: 13px;
  font-weight: 300;
  line-height: 17px;
`

interface WidgetToggleProps {
  isAdd: boolean
  float: boolean
}

export const StyledWidgetToggle = styled<WidgetToggleProps, 'button'>('button')`
  color: white;
  font-weight: 600;
  font-size: 13px;
  padding: 10px 25px;
  border-radius: 100px;
  float: ${p => p.float ? 'right' : 'none'};
  margin-right: ${p => p.float ? 10 : 0}px;
  border: none;
  margin-top: 8px;
  cursor: pointer;
  background: ${p => p.isAdd ? '#FB542B' : '#212529'};
`

export const StyledButtonIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: sub;
  margin-right: 5px;
  height: 20px;
  width: 20px;
`

export const StyledButtonLabel = styled<{}, 'span'>('span')`
  max-width: 100px;
  text-overflow: ellipsis;
  display: inline-block;
  overflow: hidden;
  white-space: nowrap;
`
