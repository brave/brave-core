// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding-top: 25px;
`

export const PanelTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.04em;
  color: ${(p) => p.theme.color.text01};
  margin-top: 5px;
`

export const FavIcon = styled.img`
  width: 48px;
  height: 48px;
  border-radius: 5px;
  background-color: ${(p) => p.theme.palette.grey200};
  margin-bottom: 7px;
`
