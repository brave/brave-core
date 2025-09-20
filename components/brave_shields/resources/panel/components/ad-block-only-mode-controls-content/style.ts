// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Container = styled.div`
  padding: 0 var(--leo-spacing-xl) var(--leo-spacing-xl);
`

export const HeaderRow = styled.div`
  margin: 0 var(--leo-spacing-xl) var(--leo-spacing-xl);
  display: flex;
  align-items: center;
  gap: var(--leo-spacing-m);
`

export const HeaderTextColumn = styled.div`
  display: flex;
  flex-direction: column;
`

export const HeaderTitle = styled.div`
  font: var(--leo-font-default-regular);
  color: var(--leo-color-text-primary);
  margin-bottom: var(--leo-spacing-xs);
`

export const HeaderDescription = styled.div`
  font: var(--leo-font-small-regular);
  color: var(--leo-color-text-tertiary);
`

export const AlertWrapper = styled.div`
  padding-top: var(--leo-spacing-m);
`

export const AlertTitle = styled.div`
  font: var(--leo-font-heading-h4);
`

export const AlertDescription = styled.div`
  font: var(--leo-font-default-regular);
`

export const Actions = styled.div`
  padding-top: var(--leo-spacing-xl);
  display: flex;
  gap: var(--leo-spacing-m);
`
