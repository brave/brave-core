// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

import favoritesSelectedLight from './assets/favorites-selected.png'
import favoritesUnselectedLight from './assets/favorites-unselected.png'
import frecencySelectedLight from './assets/frecency-selected.png'
import frecencyUnselectedLight from './assets/frecency-unselected.png'

import favoritesSelectedDark from './assets/favorites-selected-dark.png'
import favoritesUnselectedDark from './assets/favorites-unselected-dark.png'
import frecencySelectedDark from './assets/frecency-selected-dark.png'
import frecencyUnselectedDark from './assets/frecency-unselected-dark.png'

interface Props {
  textDirection: string
}

const isDarkTheme = (p: any) => {
  return p.theme.name === 'Brave Dark'
}

const getTopSiteCustomizationImage = (dark: boolean, selected: boolean, favorites: boolean) => {
  if (dark) {
    if (selected) {
      return favorites ? favoritesSelectedDark : frecencySelectedDark
    } else {
      return favorites ? favoritesUnselectedDark : frecencyUnselectedDark
    }
  } else {
    if (selected) {
      return favorites ? favoritesSelectedLight : frecencySelectedLight
    } else {
      return favorites ? favoritesUnselectedLight : frecencyUnselectedLight
    }
  }
}

export const SettingsMenu = styled('div')<Props>`
  width: 720px;
  min-width: 720px;
  ${p => p.textDirection && (p.textDirection === 'rtl') ? `left: 12px` : `right: 12px`};
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
  border-radius: 8px;
  padding: 24px;
  padding-bottom: 0px;
  box-shadow: 0px 4px 24px 0px rgba(0, 0, 0, 0.24);
  font-family: ${p => p.theme.fontFamily.body};
`

export const SettingsContent = styled('div')<{}>`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-gap: 20px;

  @media screen and (max-width: 1150px) {
    grid-gap: 0px;
  }
`

export const SettingsSidebar = styled('aside')<{}>`
  position: relative;
  /* normalize against SettingsMenu default padding */
  margin-left: -24px;
  padding-left: 24px;
`

interface SettingsSidebarActiveButtonSliderProps {
  translateTo: number
}

export const SettingsSidebarActiveButtonSlider =
  styled('div')<SettingsSidebarActiveButtonSliderProps>`
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

export const SettingsSidebarSVGContent = styled('div')<SettingsSidebarSVGContentProps>`
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

export const SettingsSidebarButtonText = styled('span')<{ isActive: boolean }>`
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
              color var(--sidebar-button-transition-timing) ease-in-out,
              font-weight var(--sidebar-button-transition-timing) ease-in-out;

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

export const SettingsSidebarButton = styled('button')<SettingsSidebarButtonProps>`
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

  &:active,
  &:focus {
    outline: none;
  }

  &:active {
    --active-opacity: 1;
  }

  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
  }
`

export const SettingsFeatureBody = styled('section')<{}>`
  padding: 10px 16px;
  height: 360px;
  overflow: auto;
  overscroll-behavior: contain;
`

export const SettingsTitle = styled('div')<{}>`
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
  }
`

export const SettingsCloseIcon = styled('button')<{}>`
  appearance: none;
  padding: 0;
  margin: 0;
  border: 0;
  width: 20px;
  height: 20px;
  cursor: pointer;
  background: inherit;
  outline: none;

  &:active {
    color: ${p => p.theme.color.brandBraveActive};
  }

  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 2px;
  }
`

interface SettingsRowProps {
  isChildSetting?: boolean
  isInteractive?: boolean
}

export const SettingsRow = styled('div')<SettingsRowProps>`
  box-sizing: border-box;
  margin-bottom: 10px;
  display: grid;
  grid-template-columns: auto max-content;
  align-items: center;
  width: 100%;
  font-weight: normal;
  ${p => p.isChildSetting && css`
    padding-left: 36px;
  `}
  ${p => p.isInteractive && css`
    cursor: pointer;
    &:focus-visible {
      outline: solid 1px ${p => p.theme.color.brandBrave};
    }
    &:hover {
      color: ${p => p.theme.color.brandBraveInteracting}
    }
    &:active {
      color: ${p => p.theme.color.brandBraveActive}
    }
  `}

  /* TODO(petemill): Use specific Component for Content and Control sides */
  :last-child:not(:first-child)
  {
    justify-self: end;
    margin-bottom: 0;
  }
`

export const SettingsText = styled('span')<{}>`
  display: flex;
  align-items: center;
  font-style: normal;
  font-size: 13px;
  line-height: 24px;
  letter-spacing: 0.01em;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const SettingsSectionTitle = styled('h3')`
  margin: 0 0 8px 0;
  font-weight: 800;
  font-size: 13px;
  line-height: 24px;
`

interface SettingsWrapperProps {
  textDirection: string
}

export const SettingsWrapper = styled('div')<SettingsWrapperProps>`
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 5;
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

export const SettingsWidget = styled('div')<{}>`
  width: calc(50% - var(--widget-gap));
  margin-top: calc(20px - var(--widget-gap));
  padding: 0px 1px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
`

export const StyledWidgetSettings = styled('div')`
  --widget-gap: 17px;
  font-family: ${p => p.theme.fontFamily.heading};
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  gap: var(--widget-gap);
`

export const FeaturedSettingsWidget = styled('div')<{}>`
  width: 100%;
`

export const StyledBannerImage = styled('img')<{}>`
  width: 100%;
  margin-bottom: 10px;
`

export const StyledSettingsInfo = styled('div')<{}>`
  float: left;
  max-width: 250px;
  flex-grow: 1;
`

export const StyledSettingsTitle = styled('div')<{}>`
  font-weight: 600;
  font-size: 14px;
  margin-bottom: 5px;
`

export const StyledSettingsCopy = styled('div')<{}>`
  font-size: 13px;
  font-weight: 300;
  line-height: 17px;
`

interface WidgetToggleProps {
  isAdd: boolean
  float: boolean
}

export const StyledWidgetToggle = styled('button')<WidgetToggleProps>`
  color: white;
  font-weight: 600;
  font-size: 13px;
  padding: 10px 25px;
  border-radius: 100px;
  float: ${p => p.float ? 'right' : 'none'};
  margin-right: ${p => p.float ? 10 : 0}px;
  border: none;
  margin-top: 15px;
  cursor: pointer;
  background: ${p => p.isAdd ? '#FB542B' : isDarkTheme(p) ? '#5E6175' : '#212529'};
  width: fit-content;
  display: flex;
  align-items: center;

  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
  }
`

export const StyledButtonIcon = styled('div')<{}>`
  display: inline-block;
  margin-right: 5px;
  width: 19px;
  height: 17px;
`

export const StyledAddButtonIcon = styled(StyledButtonIcon)`
  width: 19px;
  height: 19px;
`

export const StyledHideButtonIcon = styled(StyledButtonIcon)`
  width: 19px;
  height: 17px;
`

export const StyledButtonLabel = styled('span')<{}>`
  max-width: 100px;
  text-overflow: ellipsis;
  display: inline-block;
  white-space: nowrap;
`

export const ToggleCardsWrapper = styled('div')<{}>`
  padding: 15px 15px 5px 15px;
  clear: both;
  border-radius: 10px;
  background: ${p => isDarkTheme(p) ? '#3B3F4E' : '#F3F3FE'};
  color: ${p => isDarkTheme(p) ? '#FFF' : 'rgb(59, 62, 79)'};
`

export const ToggleCardsText = styled('div')<{}>`
  text-align: left;
  max-width: 325px;
`

export const ToggleCardsTitle = styled('span')<{}>`
  font-weight: bold;
`

export const ToggleCardsCopy = styled('p')<{}>`
  line-height: 18px;
  font-weight: normal;
`

export const ToggleCardsSwitch = styled('div')<{}>`
  float: right;
  margin: -65px -10px 0 0;
`

export const StyledTopSitesCustomizationSettings = styled('div')`
  --widget-gap: 10px;
  font-family: ${p => p.theme.fontFamily.heading};
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  margin-top: 16px;
  gap: var(--widget-gap)
`

export const StyledTopSitesCustomizationSettingsOption = styled('button')<{}>`
  width: calc(50% - var(--widget-gap) / 2);
  display: flex;
  flex-direction: column;
  padding: 0;
  border: unset;
  outline: unset;
  background: inherit;

  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
  }
`

interface CustomizationImageBorderProps {
  selected: boolean
}

export const StyledTopSitesCustomizationImageBorder = styled('div')<CustomizationImageBorderProps>`
  margin-bottom: 8px;
  border-radius: 11px;

  ${p => p.selected && css`
    background: ${p => isDarkTheme(p) ? 'linear-gradient(314.42deg, #FA7250 6.04%, #FF1893 44.31%, #A78AFF 100%)'
                                      : 'linear-gradient(122.53deg, #4C54D2 0%, #BF14A2 56.25%, #F73A1C 100%)'};
    padding: 3px;
  `}

  ${p => !p.selected && css`
    border: 2px solid ${p => isDarkTheme(p) ? '#3B3E4F' : '#F1F3F5'};
    background: ${p => isDarkTheme(p) ? '#17171F' : '#FFF'};
    padding: 1px;

    &:hover {
      border: 2px solid ${p => isDarkTheme(p) ? '#C2C4CF' : '#AEB1C2'};
      background: ${p => isDarkTheme(p) ? '#2B2D3F' : '#FAFAFF'};
      padding: 1px;
    }
  `}
`

interface CustomizationImageProps {
  isFavorites: boolean
  selected: boolean
}

export const StyledTopSitesCustomizationImage = styled('img')<CustomizationImageProps>`
  width: 100%;
  height: 100%;
  cursor: pointer;
  content: url(${p => getTopSiteCustomizationImage(isDarkTheme(p), p.selected, p.isFavorites)});

  ${p => p.selected && css`
    background: ${p => isDarkTheme(p) ? '#525779' : '#F0F2FF'};
    border-radius: 8px;
  `}
`

export const StyledTopSitesCustomizationOptionTitle = styled('div')<{}>`
  font-weight: 500;
  font-size: 13px;
  line-height: 20px;
  margin-bottom: 2px;
  text-align: left;
`

export const StyledTopSitesCustomizationOptionDesc = styled('div')<{}>`
  font-weight: 400;
  font-size: 11px;
  line-height: 17px;
  text-align: left;
`
