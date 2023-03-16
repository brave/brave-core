// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// styles
import { Column, Row } from '../../../../components/shared/style'

export const Title = styled.p<{ maxWidth?: string }>`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 28px;
  line-height: 42px;
  display: flex;
  align-items: center;
  text-align: center;
  color: ${(p) => p.theme.color.text01};
  max-width: ${(p) => p?.maxWidth || 'unset'};
`

export const LearnMoreLink = styled.a`
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.text03};
  display: flex;
  flex-direction: column;
  justify-content: center;
  height: 40px;
  text-decoration: none;
`

export const ButtonContainer = styled(Row)`
  flex: 1;
  flex-wrap: wrap;
  gap: 20px;
`

export const BlockQuote = styled.blockquote`
  display: flex;
  flex-direction: row;
`

export const VerticalRule = styled.div`
  width: 1px;
  min-height: 90%;
  background-color: ${(p) => p.theme.color.divider01};
  border-color: ${(p) => p.theme.color.divider01};
  border-width: 3px;
  border-style: solid;
  margin-right: 24px;
  border-radius: 25px;
`

export const BlockQuoteTextContainer = styled(Column)`
  font-family: Poppins;
  font-style: normal;
  color: ${(p) => p.theme.color.text02};
  text-align: left;
  align-items: flex-start;
  gap: 12px;
`

export const SubDivider = styled.div`
  width: 234px;
  height: 1px;
  background-color: ${(p) => p.theme.color.divider01};
  margin-bottom: 33px;
  margin-top: 33px;
`

export const SubDividerText = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text02};
  padding-left: 24px;
  padding-right: 24px;
`
