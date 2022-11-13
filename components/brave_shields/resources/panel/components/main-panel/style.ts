// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled, { css } from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import globalIconUrl from '../../../../../web-components/icons/globe.svg'
import managedIconUrl from '../../../../../web-components/icons/managed.svg'

interface CaratIconProps {
  isExpanded: boolean
}

interface ShieldsIconProps {
  isActive: boolean
}

export const Box = styled.div`
  background-color: ${(p) => p.theme.color.background02};
  width: 100%;
  height: 100%;
  font-family: ${(p) => p.theme.fontFamily.heading};

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    color: ${(p) => p.theme.color.interactive05};
    text-decoration: underline;
  }
`

export const HeaderBox = styled.section`
  background-color: ${(p) => p.theme.color.background03};
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
  color: ${(p) => p.theme.color.text01};
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
  color: ${(p) => p.theme.color.text01};
  grid-column: 3;
  text-align: right;
`

export const BlockNote = styled.span`
  font-size: 14px;
  font-weight: 400;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
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
  --fill-color: #5E6175;
  width: 100%;
  height: auto;
  grid-column: 1;

  svg > path {
    fill: var(--fill-color);
  }

  ${p => p.isActive && css`
    --fill-color: ${(p) => p.theme.color.interactive02};
  `}
`

export const StatusText = styled.div`
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
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
  color: ${(p) => p.theme.color.text01};
  text-align: left;
  grid-column: 2;

  span {
    color: ${(p) => p.theme.color.text03};
    display: block;
  }
`

export const ReportSiteBox = styled.div`
  grid-column: 2;

  p {
    max-width: 35ch;
    margin: 0 0 10px 0;
    color: ${(p) => p.theme.color.warningIcon};
  }
`

export const ReportSiteAction = styled.div`
  display: flex;
  flex-direction: column;

  span {
    font-weight: 500;
    font-size: 14px;
    color: ${(p) => p.theme.color.text03};
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
  --svg-color: ${(p) => p.theme.color.interactive05};
  --text-color: ${(p) => p.theme.color.interactive06};

  background-color: ${(p) => p.theme.color.background03};
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
    --text-color: ${(p) => p.theme.color.interactive07};
    --svg-color: ${(p) => p.theme.color.interactive08};
  }

  &:focus-visible {
    --border: 3px solid ${(p) => p.theme.color.focusBorder};
  }
`

export const GlobalDefaultsButton = styled(AdvancedControlsButton)``

export const CaratIcon = styled(CaratStrongDownIcon)<CaratIconProps>`
  --rotate: rotate(0deg);

  width: 16px;
  height: 16px;
  transform: var(--rotate);

  ${p => p.isExpanded && css`
    --rotate: rotate(180deg);
  `}
`

export const GlobeIcon = styled.i`
  display: block;
  width: 17px;
  height: 17px;
  background-image: url(${globalIconUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  margin-right: 5px;
`

export const ManagedIcon = styled.i`
  display: block;
  width: 17px;
  height: 17px;
  background-image: url(${managedIconUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  margin-right: 5px;
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

  /* Light Theme/Brand/interactive06 */
  color: #737ADE;

  /* Inside auto layout */
  flex: none;
  order: 1;
  flex-grow: 0;
`
