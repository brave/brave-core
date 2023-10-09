// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { HeroArticle as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import { openArticle } from './Article';
import ArticleMetaRow from './ArticleMetaRow';
import Card, { Title } from './Card';
import Link from '$web-common/Link';

interface Props {
  info: Info
}

const HeroImage = styled.img`
  width: 100%;
  height: 269px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

export default function HeroArticle({ info }: Props) {
  const { url, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url, {
    useCache: true,
    rootElement: document.body,
    rootMargin: '0px 0px 200px 0px'
  })
  return <Card onClick={() => openArticle(info.data)} ref={setElementRef}>
    <HeroImage src={url} />
    <ArticleMetaRow article={info.data} />
    <Title>
      <Link href={info.data.url.url}>{info.data.title}</Link>
    </Title>
  </Card>
}
