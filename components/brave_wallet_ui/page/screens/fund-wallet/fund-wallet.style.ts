// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const ScrollContainer = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
  overflow-y: scroll;
`

export const SelectAssetWrapper = styled.div`
  display: flex;
  width: 100%;
  align-items: stretch;
  justify-content: center;
  flex-direction: column;
`

export const SearchWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
  position: relative;
`

export const QRCodeImage = styled.img`
  width: 260px;
  height: 260px;
  border-radius: 4px;
  border: 4px solid ${(p) => p.theme.color.text01};
  margin-bottom: 16px;
`

export const AddressText = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  letter-spacing: 0.01em;
`
export const AddressTextLabel = styled(AddressText)`
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
`
