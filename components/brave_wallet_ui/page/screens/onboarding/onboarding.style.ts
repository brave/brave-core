// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const MainWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const StyledWrapper = styled.div`
  width: 376px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const TitleAndDescriptionContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin-bottom: 24px;
`

export const Title = styled.p`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 16px;
  text-align: left;
`

export const Description = styled.p`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  text-align: left;
`

export const NextButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-self: center;
  align-items: center;
  justify-content: center;
  width: 100px;
  margin-bottom: 28px;
`
