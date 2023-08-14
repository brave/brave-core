// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import Card from './Card';
import { PromotedArticle } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';

interface Props {
  info: PromotedArticle
}

const Container = styled(Card)`
  min-height: 200px;
  display: flex;
  align-items: center;
  justify-content: center;
`

export default function Advert(props: Props) {
  return <Container>
    This is a useful & relevant advert
  </Container>
}
