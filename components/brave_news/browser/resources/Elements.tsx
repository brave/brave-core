import * as React from "react";
import { Elements } from "./model";
import AdCard from "./adCard";
import HeroCard from "./heroCard";
import DiscoverCard from "./discoveryCard";
import InlineCard from "./inlineCard";
import { Signal } from "../../../../../out/Component/gen/brave/components/brave_news/common/brave_news.mojom.m";

export default function Elements({ elements, signals }: { elements: Elements[], signals: {[key:string]: Signal} }) {
  return <>
    {elements.map(e => {
      if (e.type === 'advert') return <AdCard />
      if (e.type === 'hero') return <HeroCard article={e.article} signal={signals[e.article.url.url]} />
      if (e.type === 'cluster') return <>
        {e.clusterType.type}: {e.clusterType.id}
        <Elements elements={e.elements} signals={signals} />
        </>
      if (e.type === 'discover') return <DiscoverCard sources={e.publishers} />
      if (e.type === 'inline') return <InlineCard article={e.article} isDiscover={e.isDiscover} signal={signals[e.article.url.url]} />
      return null
    })}
  </>
}
