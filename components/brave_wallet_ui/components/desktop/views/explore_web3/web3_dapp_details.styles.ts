// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const DappCategoryLabel = styled.div`
  display: inline-flex;
  color: ${leo.color.gray[50]};
  font-family: Inter, 'Poppins';
  font-size: 10px;
  font-style: normal;
  font-weight: 700;
  line-height: normal;
  text-transform: uppercase;
  height: 20px;
  padding: 0px ${leo.spacing.s};
  align-items: center;
  justify-content: center;
  gap: ${leo.spacing.xs};
  border-radius: ${leo.radius.s};
  background: ${leo.color.gray[20]};
  vertical-align: middle;
`

export const DappMetricContainer = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  gap: ${leo.spacing.s};
  padding: ${leo.spacing.xl};
  border-radius: ${leo.radius.m};
  border: 1px solid ${leo.color.divider.subtle};
`
