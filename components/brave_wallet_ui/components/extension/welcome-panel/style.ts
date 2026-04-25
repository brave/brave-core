// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
  padding: 24px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
  margin-bottom: 12px;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 270px;
  text-align: center;
  margin-bottom: 35px;
  letter-spacing: 0.01em;
`
