// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import Flex from '$web-common/Flex'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'

const Container = styled(Flex)`
  padding: ${spacing['2Xl']} 0;
`

const Header = styled.h3`
  padding: 0;
  margin: 0;
  font: ${font.heading.h2};
  color: ${color.text.secondary};
`

const Subtitle = styled.p`
  padding: 0;
  margin: 0;
  max-width: 66ch;
  text-align: center;
  font: ${font.default.regular};
  color: ${color.text.secondary};
`

export default function DisabledPlaceholder() {
  return (
    <Container align="center" justify="center" direction="column" gap={spacing['2Xl']}>
      <Header>
        {getLocale(S.BRAVE_NEWS_INTRO_TITLE)}
      </Header>
      <Subtitle>
        {getLocale(S.BRAVE_NEWS_INTRO_DESCRIPTION)}
      </Subtitle>
    </Container>
  )
}
