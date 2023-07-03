import * as React from "react";
import { Info } from "./buildFeed";
import { Publisher, Signal } from "../../../brave_new_tab_ui/api/brave_news";
import Card from "./card";
import Flex from '../../../brave_new_tab_ui/components/Flex.tsx'
import Dropdown from '@brave/leo/react/dropdown'

function Details({ publisher, signal }: { signal: Signal, publisher: Publisher }) {
  if (!publisher || !signal) return null
  return <Card>
    <h2>{publisher.publisherName}</h2>
    <div>
      <b>Blocked?</b>: {signal.blocked.toString()}
    </div>
    <div><b>Publisher Subscribed?</b>: {signal.sourceSubscribed.toString()}</div>
    <div><b>Channel Subscribed?</b>: {signal.channelSubscribed.toString()}</div>
    <div><b>Visit Weight</b>: {signal.sourceVisits}</div>
    <div>
      <b>Visited Urls:</b> ({signal.visitUrls.length})
      <ul>
        {signal.visitUrls.map(v => <li key={v}><a href={v}>{v}</a></li>)}
      </ul>
    </div>
  </Card>
}

export default function SignalDetails(props: Pick<Info, 'publishers' | 'signals'>) {
  const [filter, setFilter] = React.useState('')
  const [statusFilter, setStatusFilter] = React.useState<'any' | 'blocked' | 'publisher' | 'channel' | 'subscribed'>('any')

  const lowerFilter = filter.toLowerCase()
  const filtered = Object.values(props.publishers)
    .filter(p => p.publisherName.toLowerCase()?.includes(lowerFilter))
    .filter(p => {
      if (statusFilter === 'any') return true
      const s = props.signals[p.publisherId]
      if (statusFilter === 'blocked' && s.blocked) return true
      if (statusFilter === 'publisher' && s.sourceSubscribed) return true
      if (statusFilter === 'channel' && s.channelSubscribed) return true
      if (statusFilter === 'subscribed' && (s.channelSubscribed || s.sourceSubscribed)) return true
      return false
    })
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
    <Dropdown value={statusFilter} onChange={e => setStatusFilter(e.detail.value)}>
      <span slot="label">
        Show only with subscription via:
      </span>
      <leo-option>any</leo-option>
      <leo-option>publisher</leo-option>
      <leo-option>channel</leo-option>
      <leo-option>subscribed</leo-option>
      <leo-option>blocked</leo-option>
    </Dropdown>
    {filtered.map(f => <Details key={f.publisherId} signal={props.signals[f.publisherId]} publisher={f} />)}
  </Flex>
}
