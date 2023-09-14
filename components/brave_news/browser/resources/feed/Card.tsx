// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from "styled-components";
import { color, font, radius, spacing } from '@brave/leo/tokens/css'

export const Header = styled.h2`
  margin: 0;

  font: ${font.primary.heading.h2};
  color: ${color.text.primary};

  --leo-icon-size: 18px;
`

export const MetaInfo = styled.h4` 
  margin: 0;

  font: ${font.primary.heading.h4};
  color: ${color.text.secondary};

  --leo-icon-size: 12px;
`

export const Title = styled.h3`
  margin: 0;

  font: ${font.primary.heading.h3};
  color: ${color.text.primary};
`

export default styled.div`
  background: rgba(255, 255, 255, 0.3);
  border-radius: ${radius.xl};
  color: ${color.text.primary};
  padding: ${spacing["2Xl"]};

  ${p => p.onClick && 'cursor: pointer'}
`
