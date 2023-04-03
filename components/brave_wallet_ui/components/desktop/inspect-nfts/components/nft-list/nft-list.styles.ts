// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const NftListWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  align-items: center;
  justify-content: center;
`

export const List = styled.div`
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  grid-gap:16px;
  box-sizing: border-box;
  width: 100%;
`
export const NftItem = styled.div`
  position: relative;
  display: flex;
  justify-content: center;
  align-items: center;
  height: 120px;
`

export const NftItemOverlay = styled.div`
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  justify-content: center;
  align-items: center;
  color: ${p => p.theme.palette.white};
  background: linear-gradient(0deg, rgba(0, 0, 0, 0.7), rgba(0, 0, 0, 0.7));
  z-index: 3;
`

export const PiningMessage = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  margin: 0;
  padding: 0;
  text-align: center;
`
