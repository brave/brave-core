import { spacing } from '@brave/leo/tokens/css';
import * as React from 'react';
import styled from 'styled-components';
import { MetaInfo } from './feed/ArticleMetaRow';
import Card, { SmallImage, Title } from './feed/Card';
import { useBraveNews } from './shared/Context';
import { useUnpaddedImageUrl } from './shared/useUnpaddedImageUrl';

const PeekingCard = styled(Card)`
  width: 540px;
  background: var(--news-dark-theme-10, rgba(255, 255, 255, 0.10));
  box-shadow: 0px 4px 4px 0px rgba(0, 0, 0, 0.10);
  backdrop-filter: blur(40px);

  display: flex;
  flex-direction: row;
  gap: ${spacing.m};
  justify-content: space-between;

  // As we scroll down the PeekingCard should fade out. At 20% scroll it should
  // be completely invisible.
  opacity: calc(1 - (var(--ntp-scroll-percent) * 5));
`

export default function Peek() {
  const { feedV2 } = useBraveNews()
  const top = feedV2?.items?.find(a => a.article || a.hero)
  const data = (top?.hero ?? top?.article)?.data
  const imageUrl = useUnpaddedImageUrl(data?.image.paddedImageUrl?.url ?? data?.image.imageUrl?.url, undefined, true)

  if (!data) return null

  return <PeekingCard>
    <div>
      <MetaInfo article={data} />
      <Title>{data.title}</Title>
    </div>
    <SmallImage src={imageUrl} />
  </PeekingCard>
}

