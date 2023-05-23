import * as React from "react";
import { Elements } from "./model";
import AdCard from "./adCard";
import HeroCard from "./heroCard";
import DiscoverCard from "./discoveryCard";
import InlineCard from "./inlineCard";

export default function Elements({ elements }: { elements: Elements[] }) {
  return <>
    {elements.map(e => {
      if (e.type === 'advert') return <AdCard />
      if (e.type === 'hero') return <HeroCard article={e.article} />
      if (e.type === 'cluster') return <>
        {e.clusterType.type}: {e.clusterType.id}
        <Elements elements={e.elements} />
        </>
      if (e.type === 'discover') return <DiscoverCard sources={e.publishers} />
      if (e.type === 'inline') return <InlineCard article={e.article} isDiscover={e.isDiscover} />
      return null
    })}
  </>
}
