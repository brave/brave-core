// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const StyledExtensionWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: #f8f9fa;
  border-radius: 4px;
  box-shadow: 0px 0px 8px rgba(0, 0, 0, 0.25);
  width: 320px;
  height: 400px;
`

export const StyledExtensionWrapperLonger = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: #f8f9fa;
  border-radius: 4px;
  box-shadow: 0px 0px 8px rgba(0, 0, 0, 0.25);
  width: 320px;
  height: 500px;
`

export const StyledWelcomPanel = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 320px;
  height: 250px;
`

export const LongWrapper = styled.div<{ padding?: string }>`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
  padding: ${(p) => p?.padding ?? '0px 12px 0px 12px'};
  position: relative;
  box-sizing: border-box;
  background-color: ${leo.color.page.background};
`

export const ConnectWithSiteWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${leo.color.page.background};
  width: 390px;
  height: 100%;
`

export const ScrollContainer = styled.div`
  flex: 1;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
  padding: 0px 12px;
  box-sizing: border-box;
`
