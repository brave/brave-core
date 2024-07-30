// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Dialog from '@brave/leo/react/dialog'
import * as leo from '@brave/leo/tokens/css/variables'

export const DappCategoryLabel = styled.div`
  display: inline-flex;
  color: ${leo.color.neutral[50]};
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
  background: ${leo.color.neutral[20]};
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

export const DappDetailDialog = styled(Dialog)`
  --leo-dialog-backdrop-background: rgba(17, 18, 23, 0.35);
  --leo-dialog-backdrop-filter: blur(8px);
  --leo-dialog-padding: ${leo.spacing['3Xl']};
`

export const Title = styled.h1`
  color: ${leo.color.text.primary};
  font: ${leo.font.heading.h2};
  margin: 0;
  padding: 0;
`
