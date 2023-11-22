// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import SecureLink, { linkClickHandler } from '$web-common/SecureLink';
import { HeroArticle as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import { useLazyUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import ArticleMetaRow from './ArticleMetaRow';
import Card, { LargeImage, Title } from './Card';

interface Props {
  info: Info
}

export default function HeroArticle({ info }: Props) {
  const { url, setElementRef } = useLazyUnpaddedImageUrl(info.data.image.paddedImageUrl?.url, {
    useCache: true,
    rootElement: document.body,
    rootMargin: '0px 0px 200px 0px'
  })
  return <Card onClick={linkClickHandler(info.data.url.url)} ref={setElementRef}>
    <LargeImage src={url} />
    <ArticleMetaRow article={info.data} />
    <Title>
      <SecureLink onClick={e => e.stopPropagation()} href={info.data.url.url}>{info.data.title}</SecureLink>
    </Title>
  </Card>
}
