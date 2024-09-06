// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import {
  color,
  effect,
  font,
  radius,
  spacing
} from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${color.container.background};
  overflow: hidden;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-itmes: flex-start;
  gap: 0px;
  align-self: stretch;
`

export const PanelHeader = styled.div`
  display: flex;
  padding: ${spacing['2Xl']} ${spacing['2Xl']} ${spacing.xl} ${spacing['2Xl']};
  align-items: center;
  gap: ${spacing.m};
`

export const Divider = styled.div`
  height: 1px;
  background: ${color.divider.subtle};
`

export const RegionList = styled.div`
  display: flex;
  flex-direction: column;

  // Give top margin instead of padding to have spacing between header
  // and selected country as selected country is scrolled to top when
  // this panel is shown. See scrollIntoView() in <RegionContent>.
  margin-top: ${spacing.m};
  padding: 0 ${spacing.xl} ${spacing.m} ${spacing.m};
  align-items: center;
  gap: ${spacing.m};
  width: 320px;
  max-height: 380px;
  overflow-y: scroll;

  // Due to vertical scrollbar's width,
  // unnecessary horizontal scroll bar is shown.
  overflow-x: hidden;

  &::-webkit-scrollbar {
    -webkit-appearance: none;
  }

  &::-webkit-scrollbar:vertical {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    border-radius: 100px;
    background-color: ${color.divider.subtle};
  }
`

export const RegionContainer = styled.div.attrs({ tabIndex: 0 })<{
  selected: boolean
  fillBackground: boolean
}>`
  width: 296px;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  align-self: stretch;
  border-radius: ${radius.m};
  border: 1px solid ${color.divider.subtle};
  background: ${color.container.background};

  ${(p) =>
    p.fillBackground &&
    (p.selected
      ? css`
          border: 1px solid ${color.divider.interactive};
          background: ${color.container.interactive};
          box-shadow: ${effect.elevation['01']};
        `
      : css`
          &:hover {
            background: ${color.container.highlight};
            box-shadow: ${effect.elevation['01']};
          }
        `)}

  & > :last-child {
    border-bottom: none;
  }
`

export const RegionCountry = styled.div`
  position: relative;
  display: flex;
  padding: ${spacing.m} ${spacing.xl};
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

export const RegionCountryLabel = styled.span<{ selected: boolean }>`
  color: ${(p) => (p.selected ? color.text.interactive : color.text.primary)};
  font: ${font.default.semibold};
  text-align: start;
  flex-grow: 1;
`

export const CountryInfo = styled.div`
  display: flex;
  padding-left: ${spacing.m};
  flex-direction: column;
  justify-content: center;
  align-items: flex-start;
  flex: 1 0 0;
`

export const CountryServerInfo = styled.div<{ selected: boolean }>`
  color: ${(p) => (p.selected ? color.text.interactive : color.text.secondary)};
  font: ${font.small.regular};
`

export const RegionCity = styled.div<{ selected: boolean }>`
  position: relative;
  display: flex;
  padding: ${spacing.l} ${spacing.xl};
  justify-content: flex-start;
  align-items: center;
  align-self: stretch;
  border-bottom: 1px solid ${color.divider.subtle};

  ${(p) =>
    p.selected
      ? css`
          background: ${color.container.interactive};
        `
      : css`
          &:hover {
            background: ${color.container.highlight};
          }
        `}
  }
`

export const RegionConnect = styled(Button)<{ right: string }>`
  opacity: 0;
  position: absolute;
  right: ${(p) => p.right};
  transition: opacity 0.3s ease-in-out;

  ${RegionCountry}:hover &, ${RegionCity}:hover & {
    opacity: 1;
  }
`

export const RegionCityInfo = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: flex-start;
  flex: 1 0 0;
`

export const RegionCityLabel = styled.div<{ selected: boolean }>`
  color: ${(p) => (p.selected ? color.text.interactive : color.text.primary)};
  font: ${font.default.regular};
`

export const CityServerInfo = styled.div<{ selected: boolean }>`
  color: ${(p) => (p.selected ? color.text.interactive : color.text.secondary)};
  font: ${font.small.regular};
`

export const HeaderLabel = styled.span`
  color: ${color.text.primary};
  font: ${font.heading.h4};
`

export const StyledIcon = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.icon.default};
`

export const StyledCheckBox = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.text.interactive};
`

export const StyledButton = styled.button`
  background-color: transparent;
  border: 0;
  width: 20px;
  height: 20px;
  padding: 0;
  cursor: pointer;
`
