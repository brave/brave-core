// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const NftGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-gap: 25px;
  box-sizing: border-box;
  width: 100%;
  @media screen and (max-width: 700px) {
    grid-template-columns: repeat(2, 1fr);
  }
  @media screen and (max-width: 545px) {
    grid-template-columns: repeat(1, 1fr);
  }
`

export const EmptyStateText = styled.div`
  text-align: center;
  padding: 30px 0;
  color: ${(p) => p.theme.color.text03};
  font-size: 14px;
  font-family: Poppins;
`
