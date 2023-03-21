// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const PanelHeader = styled.section`
  display: inline-flex;
  align-items: center;
  justify-content: flex-end;
  width: 100%;
  padding: 18px 18px 0 18px;
  box-sizing: border-box;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 0 0 25px 0;
  position: relative;
  z-index: 2;
`

export const PanelTitle = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 600;
  letter-spacing: 0.02em;
  line-height: 2.6;
  margin: 0 13px 0 0;
`

export const SettingsButton = styled.button`
  border: 0;
  background: transparent;
  cursor: pointer;

  svg {
    width: 24px;
    height: 24px;
    fill: ${(p) => p.theme.color.text02};
  }
`

export const RegionSelectorButton = styled.button`
  --region-label-color: ${(p) => p.theme.color.text01};
  --svg-color: ${(p) => p.theme.color.text01};
  --border-color: transparent;
  display: flex;
  align-items: center;
  justify-content: space-between;
  min-width: 260px;
  height: 48px;
  border-radius: 38px;
  border: 4px solid var(--border-color);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  background-color: ${(p) => p.theme.color.background02};
  padding: 0 16px;
  cursor: pointer;

  svg {
    width: 18px;
    height: 18px;
  }

  svg>path {
    fill: var(--svg-color);
  }

  &:hover {
    --region-label-color: ${(p) => p.theme.color.interactive05};
    --svg-color: ${(p) => p.theme.color.interactive05};
  }

  &:focus-visible {
    --border-color: ${(p) => p.theme.color.focusBorder};
    outline: 0;
  }
`

export const RegionLabel = styled.span`
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 400;
  line-height: 20px;
  color: var(--region-label-color);
  letter-spacing: 0.04em;
`

export const ConnectNotAllowedNote = styled.section`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  text-align: center;
  padding: 0 22px;
  margin-bottom: 20px;
  
  div {
    padding: 12px;
    background-color: ${(p) => p.theme.color.warningBackground};
    border-radius: 8px;
  }
`

export const SessionExpiredNote = styled.section`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  text-align: center;
  line-height: 24px;
  padding: 12px;
  margin: 0 24px 32px 24px;
  border-radius: 8px;
  background-color: ${(p) => p.theme.color.warningBackground};

  a {
    color: ${(p) => p.theme.color.text01};
  }
 `

export const SessionExpiredNoteTitle = styled.div`
  font-weight: 600;
`
