// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled, { css } from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import { color } from '@brave/leo/tokens/css/variables'

interface CaratIconProps {
  isExpanded: boolean
}

interface ShieldsIconProps {
  isActive: boolean
}

export const Box = styled.div`
  background-color: ${color.container.background};
  width: 100%;
  height: 100%;
  font-family: ${(p) => p.theme.fontFamily.heading};

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    color: ${color.text.interactive};
    text-decoration: underline;
  }
`

export const HeaderBox = styled.section`
  background-color: ${color.neutral[10]};
  padding: 22px 17px 22px 17px;
`

export const SiteTitleBox = styled.section`
  width: 100%;
  display: grid;
  grid-template-columns: 24px 1fr 10px;
  grid-gap: 10px;
  align-items: center;
`

export const FavIconBox = styled.i`
  width: 22px;
  height: 22px;

  img {
    width: 100%;
    height: auto;
    object-fit: contain;
  }
`

export const SiteTitle = styled.h1`
  color: ${color.text.primary};
  font-size: 20px;
  line-height: 1.4;
  font-weight: 500;
  letter-spacing: 0.02em;
  margin: 0;
  overflow: hidden;
  text-align: left;
  // We truncate long site titles to the left
  direction: rtl;
  text-overflow: ellipsis;
  white-space: nowrap;
`

export const CountBox = styled.section`
  display: grid;
  grid-template-columns: 24px 2fr 80px;
  align-items: center;
  grid-gap: 10px;
  overflow: hidden;
  margin-top: 10px;
`

export const BlockCount = styled.span`
  font-size: 38px;
  line-height: 1;
  color: ${color.text.primary};
  grid-column: 3;
  text-align: right;
`

export const BlockNote = styled.span`
  font-size: 14px;
  font-weight: 400;
  line-height: 18px;
  color: ${color.text.primary};
  grid-column: 2;
`

export const StatusBox = styled.section`
  padding: 22px 17px;
`

export const ControlBox = styled.div`
  display: grid;
  grid-template-columns: 24px 2fr 0.5fr;
  grid-gap: 10px;
  margin-bottom: 13px;
`

export const ShieldsIcon = styled.i<ShieldsIconProps>`
  --fill-color: ${color.icon.disabled};
  width: 100%;
  height: auto;
  grid-column: 1;

  svg > path {
    fill: var(--fill-color);
  }

  ${p => p.isActive && css`
    --fill-color: ${color.icon.interactive};
  `}
`

export const StatusText = styled.div`
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${color.text.primary};
  grid-column: 2;
  word-break: break-word;
  display: -webkit-box;
  -webkit-line-clamp: 4;
  -webkit-box-orient: vertical;
  overflow: hidden;

  span {
    font-weight: 600;
  }
`

export const StatusToggle = styled.div`
  grid-column: 3;
  text-align: right;
`

export const StatusFootnoteBox = styled.div`
  display: grid;
  grid-template-columns: 24px 1fr;
  grid-gap: 10px;
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
`

export const Footnote = styled.div`
  color: ${color.text.primary};
  text-align: left;
  grid-column: 2;

  span {
    color: ${color.text.tertiary};
    display: block;
  }
`

export const ReportSiteBox = styled.div`
  grid-column: 2;

  p {
    max-width: 35ch;
    margin: 0 0 10px 0;
    color: ${color.systemfeedback.warningIcon};
  }
`

export const ReportSiteAction = styled.div`
  display: flex;
  flex-direction: column;

  span {
    font-weight: 500;
    font-size: 14px;
    color: ${color.text.tertiary};
    margin-bottom: 16px;
  }
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 0 0 25px 0;
  position: relative;
  z-index: 2;
`

export const AdvancedControlsButton = styled.button`
  --border: 3px solid transparent;
  --svg-color: ${color.icon.interactive};
  --text-color: ${color.text.interactive};

  background-color: ${color.neutral[10]};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 500;
  color: var(--text-color);
  width: 100%;
  padding: 10px 17px;
  border: var(--border);
  display: grid;
  grid-template-columns: 24px 1fr 18px;
  grid-gap: 10px;
  align-items: center;
  text-align: left;
  cursor: pointer;

  i { 
    grid-column: 1;
  }
  
  span { 
    grid-column: 2;
  }

  svg > path {
    fill: var(--svg-color);
  }

  &:hover {
    --text-color: ${color.primary[60]};
    --svg-color: ${color.primary[60]};
  }

  &:focus-visible {
    --border: 3px solid ${color.divider.interactive};
  }
`

export const GlobalDefaultsButton = styled(AdvancedControlsButton)``

export const CaratIcon = styled(CaratStrongDownIcon) <CaratIconProps>`
  --rotate: rotate(0deg);

  width: 16px;
  height: 16px;
  transform: var(--rotate);

  ${p => p.isExpanded && css`
    --rotate: rotate(180deg);
  `}
`

export const ManagedText = styled.div`
  /* Confirm */
  width: 228px;
  height: 20px;

  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 20px;

  /* identical to box height, or 154% */
  text-align: center;

  color: ${color.text.interactive};

  /* Inside auto layout */
  flex: none;
  order: 1;
  flex-grow: 0;
`
