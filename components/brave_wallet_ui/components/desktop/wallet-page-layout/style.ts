// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const StyledWrapper = styled('div') <{ maintainWidth?: boolean }>`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background02};
  width: 100%;
  min-width: ${(p) => p.maintainWidth ? 'unset' : '500px'};
 `

export const StyledContent = styled('div') <{ maintainWidth?: boolean }>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  width: 100%;
  max-width: ${(p) => p.maintainWidth ? 'unset' : '1600px'};
  padding: ${(p) => p.maintainWidth ? '0px' : '32px 32px 0px 32px'};
  @media screen and (max-width: 800px) {
    flex-direction: column;
    align-items: center;
    padding: ${(p) => p.maintainWidth ? '0px' : '32px 0px 0px 0px'};
  }
 `
