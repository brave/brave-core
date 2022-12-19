// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  --box-bg: ${(p) => p.theme.color.panelBackground};
  width: 100%;
  height: 100%;
  background: var(--box-bg);
  overflow: hidden;
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  width: 100%;
  padding: 18px 18px 0 18px;
  margin-bottom: 18px;
  box-sizing: border-box;
`

export const PanelContent = styled.section`
  padding: 0 0 24px 0;
`

export const RegionList = styled.div`
  padding: 0 18px;
  max-height: 320px;
  overflow-y: scroll;

  input {
    /* on input click the browser focuses, which causes a jump
    /* https://stackoverflow.com/a/49452792
    */
    top: auto;
    left: -9999px;
  }

  &::-webkit-scrollbar {
    -webkit-appearance: none;
  }

  &::-webkit-scrollbar:vertical {
    width: 8px;
  }

  &::-webkit-scrollbar-thumb {
    border-radius: 8px;
    border: 2px solid var(--box-bg);
    background-color: ${(p) => p.theme.color.divider01};
  }

  label {
    align-items: center; // vertically aligns radio button with contents next to it
  }
`

export const RegionLabelButton = styled.button`
  display: flex;
  align-items: center;
  gap: 8px;
  background-color: transparent;
  border: 0;
  padding: 0;
  margin: 0;
  scroll-margin: 5px;
  cursor: pointer;
`

export const RegionLabel = styled.span`
  color: ${(p) => p.theme.color.text01};
  font-family: Poppins;
  font-size: 14px;
`

export const BackButton = styled.button`
  background-color: transparent;
  border: 0;
  width: 18px;
  height: 18px;
  padding: 0;
  cursor: pointer;

  svg {
    fill: ${(p) => p.theme.color.interactive05}
  }
`

export const ActionArea = styled.div`
  box-sizing: border-box;
  width: 100%;
  padding: 18px 18px 0 18px;

  button {
    width: 100%;
  }
`
