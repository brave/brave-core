// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  padding: 0px 12px;
`

export const NavOutline = styled.div`
  display: flex;
  height: 36px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-evenly;
  border: 1px solid rgba(255,255,255,0.5);
  border-radius: 12px;
  margin-bottom: 15px;
  max-width: 300px;
`
