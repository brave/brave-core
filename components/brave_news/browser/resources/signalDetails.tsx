import * as React from "react";
import { Info } from "./buildFeed";
import { Publisher, Signal } from "../../../brave_new_tab_ui/api/brave_news";
import Card from "./card";
import Flex from '../../../brave_new_tab_ui/components/Flex.tsx'

function Details({ publisher, signal }: { signal: Signal, publisher: Publisher }) {
  if (!publisher || !signal) return null
  return <Card>
    <h2>{publisher.publisherName}</h2>
    <div>
      <b>Blocked?</b>: {signal.blocked.toString()}
    </div>
    <div><b>Subscribed?</b>: {signal.sourceSubscribed.toString()}</div>
    <div><b>Visit Weight</b>: {signal.sourceVisits}</div>
    <div>
      <b>Visited Urls:</b> ({signal.visitUrls.length})
      <ul>
        {signal.visitUrls.map(v => <li key={v}>{v}</li>)}
      </ul>
    </div>
  </Card>
}

export default function SignalDetails(props: Pick<Info, 'publishers' | 'signals'>) {
  const [filter, setFilter] = React.useState('')
  const lowerFilter = filter.toLowerCase()
  const filtered = Object.values(props.publishers)
    .filter(p => p.publisherName.toLowerCase()?.includes(lowerFilter))
    .sort((a, b) => {
      const aS = props.signals[a.publisherId]
      const bS = props.signals[b.publisherId]
      if (aS.sourceVisits === bS.sourceVisits) {
        return a.publisherName.localeCompare(b.publisherName)
      }
      return bS.sourceVisits - aS.sourceVisits
    })

  return <Flex direction="column" gap={8}>
    <input type="text" value={filter} onChange={e => setFilter(e.target.value)} />
    {filtered.map(f => <Details key={f.publisherId} signal={props.signals[f.publisherId]} publisher={f} />)}
  </Flex>
}
