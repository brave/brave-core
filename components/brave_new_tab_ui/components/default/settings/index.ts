// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

import favoritesSelectedLight from './assets/favorites-selected.svg'
import favoritesUnselectedLight from './assets/favorites-unselected.svg'
import frecencySelectedLight from './assets/frecency-selected.svg'
import frecencyUnselectedLight from './assets/frecency-unselected.svg'

import favoritesSelectedDark from './assets/favorites-selected-dark.svg'
import favoritesUnselectedDark from './assets/favorites-unselected-dark.svg'
import frecencySelectedDark from './assets/frecency-selected-dark.svg'
import frecencyUnselectedDark from './assets/frecency-unselected-dark.svg'

import CheckedCircle from './assets/checked-circle.svg'
import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css/variables'

// Reverse decisions to have the controls define their margin. This helps
// fill the gap before we remove all margins from these types of controls.
// Usually containers will want to define spacing based on specific UI needs.
interface ControllableLayoutProps {
  isLayoutControlled?: boolean
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

export const SettingsContent = styled('div')`
  display: grid;
  grid-template-columns: auto 1fr;
  grid-gap: 20px;

  @media screen and (max-width: 1150px) {
    grid-gap: 0px;
  }
`


export const SettingsFeatureBody = styled('section')`
  padding: 10px 16px;
  height: 360px;
  overflow: auto;
  overscroll-behavior: contain;
`

export const SettingsTitle = styled('h1')`
    margin: 0;
    margin-bottom: ${spacing.xl};
    font: ${font.heading.h3};
    color: ${color.text.secondary};
`

interface SettingsRowProps extends ControllableLayoutProps {
  isChildSetting?: boolean
  isInteractive?: boolean
}

export const SettingsRow = styled('div') <SettingsRowProps>`
  box-sizing: border-box;
  margin-bottom: ${p => !p.isLayoutControlled && '10px'};
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

export const SettingsText = styled('span')`
  display: flex;
  align-items: center;
  font-style: normal;
  font-size: 13px;
  line-height: 24px;
  letter-spacing: 0.01em;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const SettingsSectionTitle = styled('h3') <ControllableLayoutProps>`
  margin: 0 0 ${p => p.isLayoutControlled ? '0' : '8px'} 0;
  font-weight: 800;
  font-size: 13px;
  line-height: 24px;
`

export const SettingsWidget = styled('div')`
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

export const FeaturedSettingsWidget = styled('div')`
  width: 100%;
`

export const StyledBannerImage = styled('img')`
  width: 100%;
  margin-bottom: 10px;
`

export const StyledSettingsInfo = styled('div')`
  float: left;
  max-width: 250px;
  flex-grow: 1;
`

export const StyledSettingsTitle = styled('div')`
  font-weight: 600;
  font-size: 14px;
  margin-bottom: 5px;
`

export const StyledSettingsCopy = styled('div')`
  font-size: 13px;
  font-weight: 300;
  line-height: 17px;
`

export const StyledButtonIcon = styled('div')`
  display: inline-block;
  margin-right: 5px;
  width: 19px;
  height: 17px;
`

export const ToggleCardsWrapper = styled('div')`
  padding: 15px 15px 5px 15px;
  clear: both;
  border-radius: 10px;
  background: ${p => isDarkTheme(p) ? '#3B3F4E' : '#F3F3FE'};
  color: ${p => isDarkTheme(p) ? '#FFF' : 'rgb(59, 62, 79)'};
`

export const ToggleCardsText = styled('div')`
  text-align: left;
  max-width: 325px;
`

export const ToggleCardsTitle = styled('span')`
  font-weight: bold;
`

export const ToggleCardsCopy = styled('p')`
  line-height: 18px;
  font-weight: normal;
`

export const ToggleCardsSwitch = styled('div')`
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

export const StyledTopSitesCustomizationSettingsOption = styled('button')`
  width: calc(50% - var(--widget-gap) / 2);
  display: flex;
  flex-direction: column;
  padding: ${spacing.xs};
  border: unset;
  border-radius: ${radius.m};
  outline: unset;
  background: inherit;

  &:focus-visible {
    box-shadow: ${effect.focusState};
  }
`


interface CustomizationImageProps {
  isFavorites: boolean
  selected?: boolean
}

export const StyledTopSitesCustomizationImage = styled('img') <CustomizationImageProps>`
  width: 100%;
  cursor: pointer;
  content: url(${p => getTopSiteCustomizationImage(isDarkTheme(p), p.selected ?? false, p.isFavorites)});
  margin-bottom: ${spacing.m};

  ${p => p.selected && css`
    background: ${p => isDarkTheme(p) ? '#525779' : '#F0F2FF'};
    border-radius: 8px;
  `}
`

export const StyledTopSitesCustomizationOptionTitle = styled('div')`
  font-weight: 500;
  font-size: 13px;
  line-height: 20px;
  margin-bottom: 2px;
  text-align: left;
`

export const StyledTopSitesCustomizationOptionDesc = styled('div')`
  font-weight: 400;
  font-size: 11px;
  line-height: 17px;
  text-align: left;
`

export const StyledCustomBackgroundSettings = styled('div')`
  --option-gap: 10px;
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  margin-top: 16px;
  gap: var(--option-gap)
`

export const StyledCustomBackgroundOption = styled('button')`
  width: calc(50% - var(--option-gap) / 2);
  display: flex;
  flex-direction: column;
  padding: 0;
  cursor: pointer;
  border: unset;
  outline: unset;
  background: inherit;
  gap: 8px;

  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
  }
`

interface SelectionProps {
  selected: boolean
  removable?: boolean
}

interface ColoredBackgroundProps {
  colorValue: string
}

interface ImageBackgroundProps {
  image: string
}

export const StyledSelectionBorder = styled('div') <SelectionProps>`
  position: relative;
  width: 100%;
  height: 160px;
  ${p => p.selected && css`
    background: ${p => isDarkTheme(p)
      ? 'linear-gradient(314.42deg, #FA7250 6.04%, #FF1893 44.31%, #A78AFF 100%)'
      : 'linear-gradient(122.53deg, #4C54D2 0%, #BF14A2 56.25%, #F73A1C 100%)'};
    padding: 2px;
    border-radius: 10px;
    &::after {
      content: url(${CheckedCircle});
      position:absolute;
      top: 10px;
      right: 10px;
    }
  `}

  ${p => p.removable && css`
    &:hover::after { display: none; }
  `}
`
export const StyledUploadIconContainer = styled('div') <SelectionProps>`
  display: flex;
  flex-direction: column;
  width: 100%;
  height: 100%;
  align-items: center;
  justify-content: center;
  gap: 16px;
  ${p => p.selected
    ? css`    
      background: ${p => isDarkTheme(p) ? '#525779' : '#F0F2FF'}; 
      border-radius: 8px;`
    : css`
      border: 2px solid #E9E9F4; 
      border-radius: 10px; 
  `}
`

export const StyledCustomBackgroundOptionImage = styled('div') <SelectionProps & ImageBackgroundProps>`
  width: 100%;
  height: 100%;
  background-repeat: no-repeat;
  background-clip: padding-box;
  background-position: center;
  background-size: cover;
  ${p => p.selected
    ? css`border-radius: 8px;`
    : css`border-radius: 10px;`}
  background-image: url("${p => p.image}");
`

export const StyledCustomBackgroundOptionColor = styled('div') <SelectionProps & ColoredBackgroundProps>`
  width: 100%;
  height: 100%;
  ${p => p.selected
    ? css`border-radius: 8px;`
    : css`border-radius: 10px;`}
  background: ${p => p.colorValue};
`

export const StyledUploadLabel = styled('div')`
font-style: normal;
font-weight: 400;
font-size: 11px;
line-height: 17px;
`

export const StyledCustomBackgroundOptionLabel = styled('div')`
font-style: normal;
font-weight: 400;
font-size: 14px;
line-height: 20px;
`
