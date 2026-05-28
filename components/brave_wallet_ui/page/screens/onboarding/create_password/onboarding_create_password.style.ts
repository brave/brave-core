// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { Text } from '../../../../components/shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  padding-top: 32px;
  padding-left: 14px;
`

export const Title = styled(Text)`
  line-height: 30px;
  margin-bottom: 10px;
  text-align: left;
`

export const Description = styled(Text)`
  display: flex;
  align-items: flex-start;
  line-height: 20px;
  font-weight: 300;
  max-width: 380px;
  text-align: left;
  margin-bottom: 24px;
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
