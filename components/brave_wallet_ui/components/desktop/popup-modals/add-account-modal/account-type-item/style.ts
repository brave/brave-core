// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  height: 100px;
`

export const InfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: column;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  margin-bottom: 5px;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  max-width: 355px;
`

export const NetworkIcon = styled.img`
  width: 45px;
  height: 45px;
  margin-right: 10px;
`

export const LeftSide = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
`
