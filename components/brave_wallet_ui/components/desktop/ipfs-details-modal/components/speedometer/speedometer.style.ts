// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const SpeedometerWrapper = styled.div`
  position: relative;
  width: 11em;
  height: 11em;
  margin-top: -1em;
  overflow: hidden;
`

export const DetailsWrapper = styled.div`
  position: absolute;
  top: 60%;
  left: 50%;
  transform: 'translate(-50%, -50%)' 
`

export const SpeedText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 22px;
  line-height: 44px;
  text-align: center;
  color: ${p => p.theme.palette.text01}
`

export const SpeedUnitText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  text-align: center;
  color: ${p => p.theme.palette.text01}
`

export const Title = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  text-align: center;
  color: ${p => p.theme.palette.text01}
`
