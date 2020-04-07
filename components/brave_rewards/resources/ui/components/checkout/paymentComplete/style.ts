/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const Container = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 43px 0 68px 0;
`

export const ImageContainer = styled.div`
  padding-right: 32px;
`

export const TextContainer = styled.div`
  padding: 18px 21px 0 0;
`

export const Header = styled.h2`
  font-size: 22px;
  font-weight: 500;
  margin: 0;
  color: ${p => p.theme.palette.neutral700};
`

export const Text = styled.p`
  margin-top: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 16px;
`
