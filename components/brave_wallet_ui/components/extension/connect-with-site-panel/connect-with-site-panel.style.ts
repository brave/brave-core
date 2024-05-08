// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import styled from 'styled-components'

// Shared Styles
import { Row, Column } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  height: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${leo.color.container.highlight};
  position: relative;
  box-sizing: border-box;
`

export const ScrollContainer = styled.div`
  display: flex;
  width: 100%;
  height: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  background-color: ${leo.color.container.highlight};
  position: relative;
  box-sizing: border-box;
  overflow-y: auto;
  overflow-x: hidden;
`

export const BackgroundContainer = styled.div<{ backgroundImage: string }>`
  display: flex;
  top: 0px;
  right: -75px;
  bottom: 0px;
  left: -75px;
  position: absolute;
  z-index: 8;
  --background-gradient: linear-gradient(
    0deg,
    rgba(255, 255, 255, 0.9),
    rgba(255, 255, 255, 0.9)
  );
  @media (prefers-color-scheme: dark) {
    --background-gradient: linear-gradient(
      0deg,
      rgba(0, 0, 0, 0.8),
      rgba(0, 0, 0, 0.8)
    );
  }
  background: var(--background-gradient), url(${(p) => p.backgroundImage});
  background-position: center;
  background-size: cover;
  filter: blur(22px);
`

export const SelectAddressContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  padding: 0px 16px;
  top: 164px;
  position: relative;
  z-index: 9;
  box-sizing: border-box;
  border-radius: 8px 8px 0px 0px;
  background-color: ${leo.color.container.background};
  box-shadow: 0px -6px 6px -1px rgba(0, 0, 0, 0.08);
`

export const WhiteSpace = styled.div`
  display: block;
  height: 50%;
  width: 100%;
  position: fixed;
  z-index: 8;
  background-color: ${leo.color.container.background};
  bottom: 0px;
`

export const PermissionsWrapper = styled(Column)`
  top: 160px;
  position: relative;
  z-index: 9;
`

export const PermissionsContainer = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: 8px;
  position: relative;
  z-index: 9;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.07);
`

export const SectionLabel = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  text-align: left;
  color: ${leo.color.text.primary};
  margin-bottom: 8px;
`

export const SectionPoint = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  text-align: left;
  color: ${leo.color.text.primary};
  max-width: 90%;
  white-space: pre-line;
`

export const BulletContainer = styled.div<{ status: 'success' | 'error' }>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
  background-color: ${(p) =>
    p.status === 'success'
      ? leo.color.systemfeedback.successBackground
      : leo.color.systemfeedback.errorBackground};
  @media (prefers-color-scheme: dark) {
    background-color: ${(p) =>
      p.status === 'success' ? leo.color.green[20] : leo.color.red[20]};
  }
  margin-right: 8px;
  border-radius: 20px;
`

export const BulletIcon = styled(Icon)<{ status: 'success' | 'error' }>`
  --leo-icon-size: 16px;
  color: ${(p) =>
    p.status === 'success'
      ? leo.color.systemfeedback.successIcon
      : leo.color.systemfeedback.errorIcon};
`

export const AddAccountText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.interactive};
`

export const AddAcountIcon = styled(Icon)`
  --leo-icon-size: 24px;
  color: ${leo.color.text.interactive};
`

export const ButtonRow = styled(Row)<{ isReadyToConnect: boolean }>`
  display: flex;
  background-color: ${leo.color.container.background};
  @media (prefers-color-scheme: dark) {
    background-color: ${(p) =>
      p.isReadyToConnect
        ? leo.color.container.background
        : leo.color.container.highlight};
  }
  position: relative;
  z-index: 10;
  box-shadow: 0px -3px 10px -1px rgba(0, 0, 0, 0.08);
`

export const IconCircle = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  background-color: ${leo.color.container.interactive};
  border-radius: 40px;
  margin-right: 8px;
`

export const NavButton = styled(Button)`
  width: 100%;
`

export const DurationLabel = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  flex-direction: row;
  background-color: ${leo.color.container.highlight};
  outline: none;
  border-radius: 8px;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  padding: 10px 8px 10px 16px;
  margin-bottom: 8px;
  color: ${leo.color.text.primary};
  border: none;
`
