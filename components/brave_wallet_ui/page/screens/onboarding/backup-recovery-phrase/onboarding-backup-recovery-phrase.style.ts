// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const LinkText = styled.a`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
`

// Phrase card
export const PhraseCard = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  width: 376px;
  background-color: red;
  align-items: center;
`

export const PhraseCardTopRow = styled.div`
  display: flex;
  flex-direction: row;
  width: 375px;
  height: 40px;
  align-items: center;
  justify-content: flex-end;
  padding: 14px 8px;
`

export const PhraseCardBottomRow = styled(PhraseCardTopRow)`
  justify-content: flex-start;
  height: 40px;
  background-color: blueviolet;
`
