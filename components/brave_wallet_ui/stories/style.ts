// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

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

export const ChildComponentWrapper = styled.div`
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
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
  background-color: ${(p) => p.theme.color.background01};
`

export const ConnectWithSiteWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
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

export const DesktopComponentWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background02};
  padding: 20px;
`

export const LineChartWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: white;
  padding: 20px;
  width: 80vw;
`

export const DesktopComponentWrapperRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background02};
  width: 800px;
  padding: 20px;
`
