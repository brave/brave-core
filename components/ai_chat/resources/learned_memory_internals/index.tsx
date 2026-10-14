// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import {
  DreamingRunRecord,
  LearnedMemoryInternals,
  MemoryEventRecord,
  TurnLookupRecord,
} from 'gen/brave/components/ai_chat/core/common/mojom/learned_memory_internals.mojom.m.js'

// brave://leo-memory-internals: the Dreaming runs and the chat turn lookups of
// the profile, with each request to the decision model and the LLM, the
// answers, and the decisions made from them. The page reads them again every
// few seconds, so an active run shows its steps as they come. A third list has
// the memory events: what the user, the memory tool and the clean-up did to
// the memories outside a run.

const REFRESH_MS = 2000
// Mojo times count microseconds since 1601-01-01 (Windows epoch).
const WINDOWS_TO_UNIX_EPOCH_MS = 11644473600000
// The steps that the run makes from a model answer, and the model calls.
const MODEL_STEPS = new Set([
  'gate',
  'sentence_decisions',
  'relations',
  'llm_call',
  'llm_relation',
  'rewrite_parsed',
  'merge_parsed',
])
// Fields that the step header shows, or that have their own view.
const HIDDEN_FIELDS = new Set([
  'step',
  't_ms',
  'entry',
  'latency_ms',
  'decision_requests',
  'purpose',
  'system_prompt',
  'request',
  'answer',
  'error',
])

type Json = null | boolean | number | string | Json[] | { [key: string]: Json }
type Step = { [key: string]: Json }

interface DecisionRequest {
  request: string
  response: string | null
  error: string
  latency_ms: number
  // Chat turns only: when the answer came, since the start of the lookup.
  arrived_ms?: number
}

interface Run {
  record: DreamingRunRecord
  steps: Step[]
}

interface MemoryEvent {
  record: MemoryEventRecord
  details: { [key: string]: Json }
}

interface Lookup {
  record: TurnLookupRecord
  steps: Step[]
  requests: DecisionRequest[]
}

// Reads the list again every REFRESH_MS. |load| gives null when learned
// memory is not available.
function usePolling<T>(load: () => Promise<T[] | null>): [boolean, T[]] {
  const [available, setAvailable] = React.useState(true)
  const [items, setItems] = React.useState<T[]>([])
  React.useEffect(() => {
    let stopped = false
    const read = async () => {
      const result = await load()
      if (!stopped) {
        setAvailable(result !== null)
        setItems(result ?? [])
      }
    }
    read()
    const timer = setInterval(read, REFRESH_MS)
    return () => {
      stopped = true
      clearInterval(timer)
    }
  }, [])
  return [available, items]
}

const api = LearnedMemoryInternals.getRemote()

function toDate(time: { internalValue: bigint }): Date {
  return new Date(
    Number(time.internalValue / BigInt(1000)) - WINDOWS_TO_UNIX_EPOCH_MS,
  )
}

function formatSeconds(ms: number): string {
  return `${(ms / 1000).toFixed(1)} s`
}

function formatTime(date: Date): string {
  return date.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
}

// The JSON text of the decision model, indented, in the order it was sent.
function prettyJson(text: string | null): string {
  if (text === null) {
    return ''
  }
  try {
    return JSON.stringify(JSON.parse(text), null, 2)
  } catch {
    return text
  }
}

function asString(value: Json | undefined): string {
  return typeof value === 'string' ? value : JSON.stringify(value)
}

// The steps of a run, split at each "turn" step. The steps before the first
// turn and after the last one (loading, the review, the end) are in groups of
// their own.
function groupByTurn(steps: Step[]): Step[][] {
  const groups: Step[][] = []
  let current: Step[] = []
  for (const step of steps) {
    const startsGroup =
      step.step === 'turn'
      || ((step.step === 'review' || step.step === 'done')
        && current.length > 0
        && current[0].step === 'turn')
    if (startsGroup && current.length > 0) {
      groups.push(current)
      current = []
    }
    current.push(step)
  }
  if (current.length > 0) {
    groups.push(current)
  }
  return groups
}

// |endMs| is the end of a chat turn lookup: an answer after it came too late.
function DecisionRequests(props: {
  requests: DecisionRequest[]
  endMs?: number
  open?: boolean
}) {
  return (
    <div className='calls'>
      {props.requests.map((request, i) => (
        <details
          key={i}
          className='call'
          open={props.open}
        >
          <summary>
            Decision model request {i + 1} · {request.latency_ms} ms
            {request.error && <span className='error'> · {request.error}</span>}
            {props.endMs !== undefined
              && request.arrived_ms !== undefined
              && request.arrived_ms > props.endMs && (
                <span className='error'>
                  {' '}
                  · came {request.arrived_ms - props.endMs} ms after the lookup
                  ended
                </span>
              )}
          </summary>
          <div className='call-body'>
            <div>
              <div className='label'>Sent</div>
              <pre>{prettyJson(request.request)}</pre>
            </div>
            <div>
              <div className='label'>Answer</div>
              <pre>{prettyJson(request.response) || '(none)'}</pre>
            </div>
          </div>
        </details>
      ))}
    </div>
  )
}

function LlmCall(props: { step: Step }) {
  const { step } = props
  return (
    <details
      className='call'
      open
    >
      <summary>
        LLM {asString(step.purpose)} · {asString(step.latency_ms)} ms
        {step.error && <span className='error'> · {asString(step.error)}</span>}
      </summary>
      <div className='call-body'>
        <div>
          <div className='label'>System prompt</div>
          <pre>{asString(step.system_prompt ?? '')}</pre>
          <div className='label'>User message</div>
          <pre>{asString(step.request ?? '')}</pre>
        </div>
        <div>
          <div className='label'>Answer</div>
          <pre>{step.answer === null ? '(none)' : asString(step.answer)}</pre>
        </div>
      </div>
    </details>
  )
}

// A short sentence for the steps that the user's choice to keep a memory as it
// is changes, or null. The fields of the step have the details.
function stepNote(step: Step): string | null {
  switch (step.step) {
    case 'relation_applied':
      // A replace or a merge that left the old text as it was.
      return typeof step.text_change_blocked_by === 'string'
        ? `Old text not changed: ${step.text_change_blocked_by}.`
        : null
    case 'keep_forever':
      return step.pending_change_dropped
        ? 'The user keeps this memory as it is. Its pending change left the review.'
        : 'The user keeps this memory as it is. Dreaming does not change it.'
    case 'release_keep_forever':
      return 'The user lets Dreaming change this memory again.'
    case 'review_applied': {
      const skipped = step.skipped_changed_since_run
      return Array.isArray(skipped) && skipped.length > 0
        ? `${skipped.length} change(s) not written: the memory changed after the run.`
        : null
    }
    case 'edited_by_user':
      return 'The user edited this memory. Its pending change left the review.'
    case 'preferences':
      return 'Preferences go with every request, up to a limit. The memories that the user keeps come first, then the newest.'
    default:
      return null
  }
}

function StepView(props: { step: Step }) {
  const { step } = props
  const fields = Object.entries(step).filter(([key]) => !HIDDEN_FIELDS.has(key))
  const requests = step.decision_requests as unknown as
    | DecisionRequest[]
    | undefined
  const note = stepNote(step)
  const blocked =
    step.step === 'relation_applied'
    && typeof step.text_change_blocked_by === 'string'
  return (
    <div
      className={`step step-${asString(step.step)}${blocked ? ' step-blocked' : ''}`}
    >
      <div className='step-header'>
        <span className='step-name'>{asString(step.step)}</span>
        <span className='muted'>
          {asString(step.t_ms)} ms
          {step.latency_ms !== undefined
            && step.step !== 'llm_call'
            && ` · took ${asString(step.latency_ms)} ms`}
        </span>
      </div>
      {note && <div className='note'>{note}</div>}
      {fields.length > 0 && (
        <pre className='fields'>
          {JSON.stringify(Object.fromEntries(fields), null, 2)}
        </pre>
      )}
      {requests && <DecisionRequests requests={requests} />}
      {step.step === 'llm_call' && <LlmCall step={step} />}
    </div>
  )
}

function RunView(props: { run: Run; modelStepsOnly: boolean }) {
  const { record, steps } = props.run
  const start = toDate(record.startTime)
  const end = record.endTime ? toDate(record.endTime) : null
  const copy = () => {
    navigator.clipboard.writeText(JSON.stringify(steps, null, 2))
  }
  return (
    <div className='run'>
      <div className='run-header'>
        <h2>
          Run {String(record.id)} · {record.trigger} · {record.status}
        </h2>
        <span className='muted'>
          {start.toLocaleString()}
          {end && ` · ${formatSeconds(end.getTime() - start.getTime())}`}
          {` · ${steps.length} steps`}
        </span>
        <button onClick={copy}>Copy trace as JSON</button>
      </div>
      {groupByTurn(steps).map((group, i) => {
        const turn = group[0].step === 'turn' ? group[0] : null
        const shown = props.modelStepsOnly
          ? group.filter((step) => MODEL_STEPS.has(asString(step.step)))
          : group
        return (
          <section
            key={i}
            className='turn'
          >
            <h3>
              {turn
                ? `Turn: ${asString(turn.text)}`
                : i === 0
                  ? 'Start of the run'
                  : 'End of the run'}
              {turn && <span className='muted'> · {asString(turn.date)}</span>}
            </h3>
            {shown.map((step, j) => (
              <StepView
                key={j}
                step={step}
              />
            ))}
          </section>
        )
      })}
    </div>
  )
}

async function loadRuns(): Promise<Run[] | null> {
  const result = await api.getDreamingRuns()
  if (!result.available) {
    return null
  }
  return result.runs.map((record) => ({
    record,
    steps: JSON.parse(record.traceJson) as Step[],
  }))
}

async function loadLookups(): Promise<Lookup[] | null> {
  const result = await api.getTurnLookups()
  if (!result.available) {
    return null
  }
  return result.lookups.map((record) => ({
    record,
    steps: JSON.parse(record.traceJson) as Step[],
    requests: JSON.parse(record.decisionRequestsJson) as DecisionRequest[],
  }))
}

async function loadEvents(): Promise<MemoryEvent[] | null> {
  const result = await api.getMemoryEvents()
  if (!result.available) {
    return null
  }
  return result.events.map((record) => ({
    record,
    details: JSON.parse(record.detailsJson) as { [key: string]: Json },
  }))
}

// What each memory event tells, for the list.
const EVENT_LABELS: { [action: string]: string } = {
  keep_forever: 'The user keeps the memory as it is',
  release_keep_forever: 'The user lets Dreaming change the memory again',
  edited: 'The user edited the memory',
  deleted: 'The user deleted the memory',
  deleted_all: 'The user deleted all learned memories',
  expired: 'The memory expired: Leo did not use it for too long',
  tool_saved: 'The memory tool saved a memory',
  tool_duplicate: 'The memory tool found the memory saved already',
}

function EventsTab() {
  const [available, events] = usePolling(loadEvents)
  return (
    <>
      <p className='muted'>
        The memory events, the newest first: the choices of the user, the saves
        of the memory tool, and the memories that expired. A Dreaming run and
        its review are in the first tab.
      </p>
      {!available && <p>Learned memory does not run in this profile.</p>}
      {available && events.length === 0 && (
        <p>No memory event yet since the browser started.</p>
      )}
      {events.map((event) => (
        <div
          key={String(event.record.id)}
          className={`step step-event-${event.record.action}`}
        >
          <div className='step-header'>
            <span className='step-name'>{event.record.action}</span>
            <span className='muted'>
              {toDate(event.record.time).toLocaleString()}
            </span>
          </div>
          <div className='note'>
            {EVENT_LABELS[event.record.action] ?? event.record.action}
            {event.record.text && `: ${event.record.text}`}
          </div>
          {Object.keys(event.details).length > 0 && (
            <pre className='fields'>
              {JSON.stringify(event.details, null, 2)}
            </pre>
          )}
        </div>
      ))}
    </>
  )
}

function DreamingTab() {
  const [available, runs] = usePolling(loadRuns)
  const [selectedId, setSelectedId] = React.useState<bigint | null>(null)
  const [modelStepsOnly, setModelStepsOnly] = React.useState(false)

  const selected = runs.find((run) => run.record.id === selectedId) ?? runs[0]

  return (
    <>
      <p className='muted'>
        The Dreaming runs, the newest first: what each step asked the decision
        model and the LLM, and what they answered.
      </p>
      <label>
        <input
          type='checkbox'
          checked={modelStepsOnly}
          onChange={(e) => setModelStepsOnly(e.target.checked)}
        />
        Only the model steps
      </label>
      {!available && <p>Learned memory does not run in this profile.</p>}
      {available && runs.length === 0 && (
        <p>No Dreaming run yet. Select "Learn from chats" in Leo settings.</p>
      )}
      {runs.length > 0 && (
        <div className='layout'>
          <nav>
            {runs.map((run) => (
              <button
                key={String(run.record.id)}
                className={run === selected ? 'selected' : ''}
                onClick={() => setSelectedId(run.record.id)}
              >
                <div>
                  #{String(run.record.id)} {run.record.trigger}
                </div>
                <div
                  className={
                    run.record.status === 'running' ? 'running' : 'muted'
                  }
                >
                  {run.record.status} ·{' '}
                  {formatTime(toDate(run.record.startTime))}
                </div>
              </button>
            ))}
          </nav>
          {selected && (
            <RunView
              run={selected}
              modelStepsOnly={modelStepsOnly}
            />
          )}
        </div>
      )}
    </>
  )
}

// The message of a lookup, for its button.
function lookupMessage(lookup: Lookup): string {
  const messages = lookup.steps[0]?.messages
  return Array.isArray(messages) && messages.length > 0
    ? asString(messages[0])
    : ''
}

function LookupView(props: { lookup: Lookup }) {
  const { record, steps, requests } = props.lookup
  const done = steps.find((step) => step.step === 'done')
  const endMs = done ? Number(done.t_ms) : undefined
  return (
    <div className='run'>
      <div className='run-header'>
        <h2>
          Chat turn {String(record.id)} ·{' '}
          {done ? asString(done.reason) : 'running'}
        </h2>
        <span className='muted'>
          {toDate(record.startTime).toLocaleString()}
          {done && ` · ${asString(done.total_ms)} ms`}
        </span>
        <a
          href={`chrome://leo-ai/${record.conversationUuid}`}
          target='_blank'
        >
          Open the chat
        </a>
      </div>
      <h3>Decision model</h3>
      {requests.length === 0 ? (
        <p className='muted'>No request (or no answer yet).</p>
      ) : (
        <DecisionRequests
          requests={requests}
          endMs={endMs}
          open
        />
      )}
      <h3>Steps</h3>
      {steps.map((step, i) => (
        <StepView
          key={i}
          step={step}
        />
      ))}
    </div>
  )
}

function TurnsTab() {
  const [available, lookups] = usePolling(loadLookups)
  const [selectedId, setSelectedId] = React.useState<bigint | null>(null)
  const selected =
    lookups.find((lookup) => lookup.record.id === selectedId) ?? lookups[0]
  return (
    <>
      <p className='muted'>
        The learned memory lookups of the last chat turns, the newest first:
        which memories the search found, what the decision model said about each
        one, and which went with the request to the chat model.
      </p>
      {!available && <p>Learned memory does not run in this profile.</p>}
      {available && lookups.length === 0 && (
        <p>No chat turn yet. Send a message in Leo.</p>
      )}
      {lookups.length > 0 && (
        <div className='layout'>
          <nav>
            {lookups.map((lookup) => (
              <button
                key={String(lookup.record.id)}
                className={lookup === selected ? 'selected' : ''}
                onClick={() => setSelectedId(lookup.record.id)}
              >
                <div className='message'>
                  #{String(lookup.record.id)} {lookupMessage(lookup)}
                </div>
                <div className={lookup.record.endTime ? 'muted' : 'running'}>
                  {lookup.record.endTime ? '' : 'running · '}
                  {formatTime(toDate(lookup.record.startTime))}
                </div>
              </button>
            ))}
          </nav>
          {selected && <LookupView lookup={selected} />}
        </div>
      )}
    </>
  )
}

function App() {
  const [tab, setTab] = React.useState<'dreaming' | 'turns' | 'events'>(
    'dreaming',
  )
  return (
    <div className='page'>
      <style>{CSS}</style>
      <header>
        <h1>Leo memory internals</h1>
        <p className='muted'>Kept in memory until the browser restarts.</p>
        <div className='tabs'>
          <button
            className={tab === 'dreaming' ? 'selected' : ''}
            onClick={() => setTab('dreaming')}
          >
            Dreaming runs
          </button>
          <button
            className={tab === 'turns' ? 'selected' : ''}
            onClick={() => setTab('turns')}
          >
            Chat turns
          </button>
          <button
            className={tab === 'events' ? 'selected' : ''}
            onClick={() => setTab('events')}
          >
            Memory events
          </button>
        </div>
      </header>
      {tab === 'dreaming' && <DreamingTab />}
      {tab === 'turns' && <TurnsTab />}
      {tab === 'events' && <EventsTab />}
    </div>
  )
}

const CSS = `
  .page { padding: 16px 24px; }
  h1 { font-size: 20px; margin: 0 0 4px; }
  h2 { font-size: 16px; margin: 0; }
  h3 { font-size: 14px; margin: 16px 0 8px; }
  .muted { color: GrayText; }
  .error { color: #d33; }
  .running { color: #2a7; font-weight: 600; }
  .layout {
    display: flex; gap: 16px; margin-top: 12px; align-items: flex-start;
  }
  nav {
    display: flex; flex-direction: column; gap: 4px; min-width: 180px;
    position: sticky; top: 8px;
  }
  nav button {
    text-align: left; padding: 6px 8px; border: 1px solid #8884;
    border-radius: 6px; background: none; color: inherit; cursor: pointer;
  }
  nav button.selected, .tabs button.selected {
    border-color: #66f; background: #66f2;
  }
  nav button .message {
    max-width: 240px; overflow: hidden; text-overflow: ellipsis;
    white-space: nowrap;
  }
  .tabs { display: flex; gap: 4px; margin: 8px 0; }
  .tabs button {
    padding: 6px 12px; border: 1px solid #8884; border-radius: 6px;
    background: none; color: inherit; cursor: pointer;
  }
  .run { flex: 1; min-width: 0; }
  .run-header {
    display: flex; gap: 12px; align-items: baseline; flex-wrap: wrap;
  }
  .turn { border-top: 1px solid #8884; }
  .step {
    margin: 6px 0 6px 12px; padding: 6px 8px; border-left: 3px solid #8886;
  }
  .step-gate, .step-sentence_decisions, .step-relations,
  .step-relevance {
    border-left-color: #c80;
  }
  .step-llm_call, .step-llm_relation { border-left-color: #66f; }
  .step-review, .step-store, .step-review_applied { border-left-color: #2a7; }
  .step-fact_dropped { border-left-color: #d33; }
  .step-keep_forever, .step-release_keep_forever, .step-preferences,
  .step-blocked, .step-event-keep_forever, .step-event-release_keep_forever {
    border-left-color: #0aa;
  }
  .step-event-edited, .step-event-tool_saved { border-left-color: #2a7; }
  .step-event-deleted, .step-event-deleted_all, .step-event-expired {
    border-left-color: #d33;
  }
  .step-event-tool_duplicate, .step-edited_by_user {
    border-left-color: #c80;
  }
  .note { margin: 2px 0; font-size: 12px; }
  .step-header { display: flex; gap: 8px; align-items: baseline; }
  .step-name { font-weight: 600; font-family: monospace; }
  pre {
    white-space: pre-wrap; word-break: break-word; margin: 4px 0;
    font-size: 12px;
  }
  .fields { background: #8881; padding: 4px 6px; border-radius: 4px; }
  .call { margin: 4px 0; }
  .call summary { cursor: pointer; }
  .call-body { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
  .call-body pre {
    background: #8881; padding: 4px 6px; border-radius: 4px;
    max-height: 360px; overflow: auto;
  }
  .label { font-weight: 600; margin-top: 4px; }
`

createRoot(document.getElementById('root')!).render(<App />)
