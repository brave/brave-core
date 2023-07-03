import * as React from "react";
import { Info } from "./buildFeed";
import { Publisher, Signal } from "../../../brave_new_tab_ui/api/brave_news";
import Card from "./card";

function Details({ publisher, signal }: { signal: Signal, publisher: Publisher }) {
  if (!publisher || !signal) return null
  return <Card>
    <h2>{publisher.publisherName}</h2>
    <b>Blocked?</b>: {signal.blocked}
    <b>Subscribed?</b>: {signal.sourceSubscribed}
    <b>Visit Weight</b>: {signal.sourceVisits}
    <ul>
      Visited Urls:
      {signal.visitUrls.map(v => <li key={v}>{v}</li>)}
    </ul>
  </Card>
}

export default function SignalDetails(props: Pick<Info, 'publishers' | 'signals'>) {
  const [filter, setFilter] = React.useState('')
  const lowerFilter = filter.toLowerCase()
  const filtered = Object.values(props.publishers)
    .filter(p => p.publisherName.toLowerCase()?.includes(lowerFilter))
    console.log(props.publishers, props.signals)
  return <div>
    <input type="text" value={filter} onChange={e => setFilter(e.target.value)} />
    {filtered.map(f => <Details key={f.publisherId} signal={props.signals[f.publisherId]} publisher={f} />)}
  </div>
}
