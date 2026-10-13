#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests learned memory (Dreaming) end to end through the UI, over CDP.

  ./learned_memory_eval.py --fresh          # new profile, all steps
  ./learned_memory_eval.py                  # resume: skips the done steps
  ./learned_memory_eval.py --only learn     # one step again
  ./learned_memory_eval.py --set relations  # another chat set

Steps:
  setup   prefs on; Leo > Models > Add model: the local LLM on Ollama.
  chats   types a chat set of ../learned_memory_eval/chat_sets into Leo,
          oldest first. Leo's default model answers.
  learn   "Learn from chats"; nothing is stored before the review; keeps every
          change; checks the memories against the set, that the index has
          each memory at its text version, and the settings search.
  modify  a chat that moves the user to Munich; "Learn from chats"; a second
          run while the review waits gives kReviewPending; edits the Munich
          change, drops the others; the edited text is stored and the index
          follows.
  recall  a chat turn gets the learned memories (turn_memory_lookup log).
  delete  the trash button on what "what food do I eat" finds; it leaves the
          list, the index and the search.

Needs Ollama on localhost:11434 with clef-flash:9b and qwen3.5:9b, which
../learned_memory_eval/ollama_setup.py loads. Dreaming sends nothing to a
server.

Open the profile by hand with ./open_profile.py learned-memory.
"""

import argparse
import datetime
import json
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import time

import cdp
import leo

PROFILE_NAME = 'leo-learned-memory-test'
EVAL_TOOLS = os.path.join(cdp.BRAVE_DIR, 'tools', 'learned_memory_eval')

LLM = 'qwen3.5:9b'
LLM_ENDPOINT = 'http://localhost:11434/v1/chat/completions'
# The switches that turn learned memory on, with its local models.
SWITCHES = [
    '--enable-features=AIChatLearnedMemory:'
    'decision_model_name/clef-flash%3A9b/'
    'decision_model_url/http%3A%2F%2Flocalhost%3A11434%2Fv1%2Fsystemone/'
    'local_llm_model_name/qwen3.5%3A9b,HistoryEmbeddings',
    '--enable-logging=stderr',
    '--vmodule=user_memory_manager=1,dreaming_run=1,turn_memory_lookup=1',
]

MOVE_CHAT = (
    'We moved again last week: we left Berlin and now live in '
    'Munich. Any good bakeries near Schwabing?'
)
EDITED_TEXT = 'Lives in Munich (Schwabing)'
# DreamNowStatus.kReviewPending in customization_settings.mojom.
REVIEW_PENDING = 6
RECALL_CHAT = 'Suggest a dinner I could cook tonight.'
# A reply that knows "Is vegetarian" names none of these.
MEAT = [
    'chicken',
    'beef',
    'pork',
    'bacon',
    'lamb',
    'steak',
    'sausage',
    'turkey',
    r'ham\b',
]
SEARCHES = [
    'where do I live',
    'my dogs',
    'what food do I eat',
    'what is my job',
    'sport I train for',
]

LEARNED = 'MEMORY-SECTION'
LEARNED_MEMORIES = """
  const r = await sec.browserProxy_.getCustomizationSettingsHandler()
      .getLearnedMemories();
  return {available: r.available, memories: r.memories.map(m => ({
    uuid: m.uuid, text: m.text, category: m.category, type: m.type,
    previous: m.previousText || null}))};"""
DREAM_NOW = """
  const b = sec.shadowRoot.querySelector('#dreamNowButton');
  if (!b || sec.isDreaming_) return false;
  b.click();
  return true;"""
DREAM_STATE = 'return {busy: sec.isDreaming_, result: sec.dreamResult_};'
REVIEW_STATE = """
  const d = sec.shadowRoot.querySelector('#dreamingReviewDialog');
  // The changes in the order of the dialog: the page keeps them in that order.
  const items = sec.reviewItems_.map(i => ({
      change: sec.getChangeLabel_(i),
      text: i.text,
      stored: sec.hasStoredText_(i) ? sec.getStoredText_(i) : null,
      keep: i.keep}));
  return {open: !!(d && d.open), pending: sec.reviewItems_.length, items,
          error: sec.reviewError_};"""
REVIEW_ITEM = (
    "sec.shadowRoot.querySelectorAll("
    "'#dreamingReviewDialog .review-item')[{index}]"
)
DREAM_NOW_STATUS = """
  const r = await sec.browserProxy_.getCustomizationSettingsHandler()
      .dreamNow();
  return r.result.status;"""
TRASH_BUTTON = """
  for (const row of sec.shadowRoot.querySelectorAll('.memory')) {{
    const label = row.querySelector('.label');
    if (row.getClientRects().length && label &&
        label.textContent.trim() === {text}) {{
      const trash = [...row.querySelectorAll('.memory-actions leo-button')]
          .find(b => b.querySelector('leo-icon[name="trash"]'));
      trash.click();
      // The page asks first.
      await new Promise(r => setTimeout(r, 500));
      sec.shadowRoot.querySelector('cr-dialog .delete-button').click();
      return true;
    }}
  }}
  return false;"""
MODEL_FORM_INPUT = """
  const input = sec.shadowRoot.querySelectorAll('leo-input')[{index}]
      .shadowRoot.querySelector('input');
  input.focus();
  input.select();
  return 1;"""
MODEL_FORM = """
  return {label: sec.label, name: sec.modelRequestName,
          url: sec.endpointUrl, invalid: sec.isUrlInvalid};"""
MODEL_FORM_SAVE = """
  sec.shadowRoot.querySelector('.actions-container leo-button').click();
  return 1;"""


class Eval:
    """The profile, the chat set, the state of the steps, and the folder of
    this run."""

    def __init__(self, profile_name, chat_set):
        self.profile = cdp.profile_path(profile_name)
        self.files = cdp.tools_dir(self.profile)
        self.state_path = os.path.join(self.files, 'learned_memory_eval.json')
        self.chat_set = chat_set
        self.state = {'done': [], 'results': {}}
        if os.path.exists(self.state_path):
            with open(self.state_path, encoding='utf-8') as f:
                self.state = json.load(f)
        now = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
        self.run_dir = os.path.join(self.files, 'runs', now)
        os.makedirs(self.run_dir, exist_ok=True)
        self.last_log = None

    def browser(self, name):
        self.last_log = os.path.join(self.run_dir, f'browser-{name}.log')
        return cdp.Browser(self.profile, self.last_log, SWITCHES)

    def save(self):
        with open(self.state_path, 'w', encoding='utf-8') as f:
            json.dump(self.state, f, indent=1)

    def index_state(self):
        """(chat DB stamps, index stamps) of the learned memories, read with
        the browser closed. Texts are encrypted; uuids and versions are not."""
        db = os.path.join(self.profile, 'Default')
        with sqlite3.connect(f'file:{db}/AIChat?mode=ro', uri=True) as c:
            chat_db = sorted(
                c.execute(
                    'SELECT uuid, text_version FROM learned_memory'
                ).fetchall()
            )
        with sqlite3.connect(
            f'file:{db}/AIChatEmbeddings?mode=ro', uri=True
        ) as c:
            index = sorted(
                c.execute(
                    'SELECT memory_uuid, text_version FROM learned_memory_passage'
                ).fetchall()
            )
        return chat_db, index


# --- the UI -----------------------------------------------------------------


def in_section(ws, tag, body):
    return ws.eval(cdp.find_in(tag, body))


def learned(ws):
    return in_section(ws, LEARNED, LEARNED_MEMORIES)


def review_state(ws):
    return in_section(ws, LEARNED, REVIEW_STATE)


def wait_for(predicate, limit=30):
    for _ in range(limit * 2):
        value = predicate()
        if value:
            return value
        time.sleep(0.5)
    raise RuntimeError('timed out')


def dream_now(ws, limit=900):
    """Selects "Learn from chats" and waits for the result line."""
    clicked = False
    for _ in range(30):  # the page can be slow while the browser indexes
        clicked = in_section(ws, LEARNED, DREAM_NOW)
        if clicked:
            break
        time.sleep(1)
    if not clicked:
        raise RuntimeError('no Learn from chats button: is learned memory on?')
    start = time.time()
    time.sleep(2)
    while time.time() - start < limit:
        state = in_section(ws, LEARNED, DREAM_STATE)
        if state and not state['busy'] and state['result']:
            state['seconds'] = round(time.time() - start)
            return state
        time.sleep(3)
    raise RuntimeError('Learn from chats did not finish')


def open_review(ws):
    state = review_state(ws)
    if state['open']:
        return state
    in_section(
        ws,
        LEARNED,
        "sec.shadowRoot.querySelector('#reviewButton').click();return 1;",
    )

    def shown():
        state = review_state(ws)
        return state if state['open'] and state['items'] else None

    return wait_for(shown)


def handle_review(ws, decide, before_save=None):
    """Reviews the changes in the dialog as a user: unchecks the ones that
    decide() drops, types the texts that it edits, and selects Save."""
    items = open_review(ws)['items']
    stored_before = len(learned(ws)['memories'])
    drop, edits = decide(items)
    for i in drop:
        item = REVIEW_ITEM.format(index=i)
        # The checkbox of the change, as a user clicks it.
        in_section(
            ws,
            LEARNED,
            f"{item}.querySelector('leo-checkbox').shadowRoot"
            ".querySelector('input').click(); return 1;",
        )
    for i, text in edits.items():
        item = REVIEW_ITEM.format(index=i)
        # The pencil opens the text box of the change, and the box gets focus.
        in_section(
            ws,
            LEARNED,
            f"{item}.querySelector('.review-edit-button').click(); return 1;",
        )
        time.sleep(0.5)
        in_section(
            ws,
            LEARNED,
            f"const t = {item}.querySelector('.review-text')"
            ".shadowRoot.querySelector('textarea');"
            't.focus(); t.select(); return 1;',
        )
        time.sleep(0.3)
        ws.call('Input.insertText', text=text)
        time.sleep(0.3)
    shown = review_state(ws)['items']
    extra = before_save(ws) if before_save else None
    in_section(
        ws,
        LEARNED,
        "sec.shadowRoot.querySelector('#reviewSaveButton').click();return 1;",
    )

    def ended():
        state = review_state(ws)
        return state if not state['open'] or state['error'] else None

    after = wait_for(ended)
    return {
        'items': items,
        'before_save': shown,
        'dropped': sorted(drop),
        'edited': {str(i): t for i, t in edits.items()},
        'stored_before_save': stored_before,
        'error': after['error'],
        'pending_after': after['pending'],
        'extra': extra,
    }


def dream_and_review(ws, decide, before_save=None, rounds=4):
    """Runs "Learn from chats" until a run finds nothing new, and reviews each
    run. A run of the daily timer can leave a review, or be active."""
    results, reviews = [], []
    for _ in range(rounds):
        if review_state(ws)['pending']:
            reviews.append(handle_review(ws, decide, before_save))
            cdp.log(
                '  review:',
                {
                    k: reviews[-1][k]
                    for k in (
                        'dropped',
                        'edited',
                        'stored_before_save',
                        'error',
                    )
                },
            )
            continue
        result = dream_now(ws)
        cdp.log('  dream now:', result)
        results.append(result)
        if 'already learns' in result['result']:
            time.sleep(20)
            continue
        if not review_state(ws)['pending']:
            break
    return results, reviews


def type_into_model_form(ws, index, text):
    in_section(ws, 'MODEL-CONFIG-UI', MODEL_FORM_INPUT.format(index=index))
    time.sleep(0.3)
    ws.call('Input.insertText', text=text)
    time.sleep(0.3)
    for kind in ('keyDown', 'keyUp'):
        ws.call(
            'Input.dispatchKeyEvent',
            type=kind,
            key='Tab',
            code='Tab',
            windowsVirtualKeyCode=9,
        )
    time.sleep(0.3)


def matches(text, rule):
    text = text.lower()
    return all(w in text for w in rule.get('all', [])) and (
        not rule.get('any') or any(w in text for w in rule['any'])
    )


def check_expectations(memories, expect):
    texts = [m['text'] for m in memories]
    rows = []
    for rule in expect.get('memories', []):
        hit = [t for t in texts if matches(t, rule)]
        rows.append(('expected', rule['label'], bool(hit), hit))
    for rule in expect.get('absent', []) + expect.get('forbidden', []):
        hit = [t for t in texts if matches(t, rule)]
        rows.append(('absent', rule['label'], not hit, hit))
    if 'max_memories' in expect:
        rows.append(
            (
                'count',
                f"at most {expect['max_memories']}",
                len(texts) <= expect['max_memories'],
                [len(texts)],
            )
        )
    return rows


# --- the steps --------------------------------------------------------------


def step_setup(run):
    os.makedirs(run.profile, exist_ok=True)
    target = os.path.join(run.profile, 'BraveLocalAIModels')
    source = os.path.join(cdp.dev_profile_path(), 'BraveLocalAIModels')
    if not os.path.exists(target):
        shutil.copytree(source, target)
    with run.browser('setup') as browser:
        for key, value in [
            ('brave.history_embeddings_enabled', True),
            ('brave.ai_chat.storage_enabled', True),
            ('brave.ai_chat.user_memory_enabled', True),
            ('session.restore_on_startup', 5),
        ]:
            if not browser.set_pref(key, value):
                raise RuntimeError(f'could not set {key}')
        ws, tab = browser.new_tab('brave://settings/leo-ai/add-model', settle=5)
        for index, text in [
            (0, 'Qwen 3.5 9B (Ollama)'),
            (1, LLM),
            (2, LLM_ENDPOINT),
        ]:
            type_into_model_form(ws, index, text)
        form = in_section(ws, 'MODEL-CONFIG-UI', MODEL_FORM)
        cdp.log('  form:', form)
        if form['name'] != LLM or form['url'] != LLM_ENDPOINT:
            raise RuntimeError(f'the form did not take the text: {form}')
        in_section(ws, 'MODEL-CONFIG-UI', MODEL_FORM_SAVE)
        time.sleep(3)
        browser.close_tab(tab)
    with open(
        os.path.join(run.profile, 'Default', 'Preferences'), encoding='utf-8'
    ) as f:
        models = json.load(f)['brave']['ai_chat'].get('custom_models', [])
    if not any(m.get('model_request_name') == LLM for m in models):
        raise RuntimeError('the custom model was not saved')
    return {'form': form, 'custom_models': models}


def step_chats(run):
    chats = sorted(run.chat_set['chats'], key=lambda c: -c.get('days_ago', 0))
    typed = run.state.setdefault('typed_chats', [])
    with run.browser('chats') as browser:
        for item in chats:
            if item['title'] in typed:
                cdp.log('  chat:', item['title'], '(typed before)')
                continue
            cdp.log('  chat:', item['title'])
            leo.chat(browser, [turn['user'] for turn in item['turns']])
            typed.append(item['title'])
            run.save()
        time.sleep(20)  # the entries reach the index
    return {'chats': [c['title'] for c in chats]}


def step_learn(run):
    with run.browser('learn') as browser:
        ws, _ = leo.open_customization(browser)
        cdp.log('  before:', learned(ws))
        # Keep every change.
        results, reviews = dream_and_review(ws, lambda items: ([], {}))
        time.sleep(15)  # the sync embeds the new memories
        memories = learned(ws)['memories']
        searches = {q: leo.search_memories(ws, q) for q in SEARCHES}
    chat_db, index = run.index_state()
    return {
        'dream': results[0],
        'dreams': results,
        'reviews': reviews,
        'memories': memories,
        'searches': searches,
        'checks': check_expectations(memories, run.chat_set['expect']),
        'chat_db': chat_db,
        'index': index,
    }


def step_modify(run):
    before = {m['uuid']: m for m in run.state['results']['learn']['memories']}

    def decide(items):
        # Keep the change about Munich, with an edited text. Drop the rest.
        keep = next(
            (
                i
                for i, item in enumerate(items)
                if 'munich' in item['text'].lower()
            ),
            None,
        )
        drop = [i for i in range(len(items)) if i != keep]
        return drop, ({} if keep is None else {keep: EDITED_TEXT})

    def while_pending(ws):
        # No run starts while the review waits.
        return in_section(ws, LEARNED, DREAM_NOW_STATUS)

    with run.browser('modify') as browser:
        if not run.state.get('move_typed'):
            cdp.log('  chat:', MOVE_CHAT)
            leo.chat(browser, [MOVE_CHAT])
            run.state['move_typed'] = True
            run.save()
        ws, _ = leo.open_customization(browser)
        results, reviews = dream_and_review(ws, decide, while_pending)
        time.sleep(15)
        memories = learned(ws)['memories']
        where = leo.search_memories(ws, 'where do I live')
    chat_db, index = run.index_state()
    changed = [
        {
            'uuid': m['uuid'],
            'before': before[m['uuid']]['text'],
            'after': m['text'],
        }
        for m in memories
        if m['uuid'] in before and m['text'] != before[m['uuid']]['text']
    ]
    return {
        'dream': results[0],
        'dreams': results,
        'reviews': reviews,
        'edited_stored': [m for m in memories if m['text'] == EDITED_TEXT],
        'memories': memories,
        'changed': changed,
        'added': [m['text'] for m in memories if m['uuid'] not in before],
        'where_do_i_live': where,
        'chat_db': chat_db,
        'index': index,
    }


def step_recall(run):
    with run.browser('recall') as browser:
        cdp.log('  chat:', RECALL_CHAT)
        reply = leo.chat(browser, [RECALL_CHAT])[0]
    with open(run.last_log, encoding='utf-8', errors='replace') as f:
        lines = [line.strip() for line in f if 'turn_memory_lookup' in line]
    meat = [w for w in MEAT if re.search(rf'\b{w}', reply, re.I)]
    return {'log': lines[-20:], 'reply': reply, 'meat_words': meat}


def step_delete(run):
    query = 'what food do I eat'
    with run.browser('delete') as browser:
        ws, _ = leo.open_customization(browser)
        memories = learned(ws)['memories']
        before = leo.search_memories(ws, query)
        hit = next((h['text'] for h in before if h['learned']), None)
        if not hit:
            raise RuntimeError(f'{query!r} finds no learned memory to delete')
        target = next(m for m in memories if m['text'] == hit)
        # The trash button of the memory, as a user clicks it.
        clicked = in_section(
            ws, LEARNED, TRASH_BUTTON.format(text=json.dumps(hit))
        )
        time.sleep(8)  # the sync drops the embedding
        after = leo.search_memories(ws, query)
        remaining = [m['uuid'] for m in learned(ws)['memories']]
    chat_db, index = run.index_state()
    return {
        'deleted': target,
        'clicked': clicked,
        'search_before': before,
        'search_after': after,
        'still_found': any(h['text'] == hit and h['learned'] for h in after),
        'still_listed': target['uuid'] in remaining,
        'chat_db': chat_db,
        'index': index,
    }


STEPS = [
    ('setup', step_setup),
    ('chats', step_chats),
    ('learn', step_learn),
    ('modify', step_modify),
    ('recall', step_recall),
    ('delete', step_delete),
]


# --- the report -------------------------------------------------------------


def hit_texts(hits):
    return '; '.join(h['text'] for h in hits) or 'nothing'


def stamps_line(result):
    chat_db, index = result['chat_db'], result['index']
    same = [tuple(x) for x in chat_db] == [tuple(x) for x in index]
    return (
        f"Index in sync with the chat DB: **{'yes' if same else 'NO'}** "
        f'({len(chat_db)} memories, {len(index)} embeddings)'
    )


def report_learn(result):
    dream = result['dream']
    out = [
        '## 1. Learn',
        '',
        f"Learn from chats: {dream['result']} ({dream['seconds']} s)",
        '',
        '| Check | Label | Pass | Matched |',
        '|---|---|---|---|',
    ]
    for kind, label, passed, hit in result['checks']:
        out.append(
            f"| {kind} | {label} | {'yes' if passed else 'NO'} | "
            f"{'; '.join(map(str, hit))} |"
        )
    out += ['', 'Reviews (all kept):', '']
    for review in result['reviews']:
        out.append(
            f"- {len(review['items'])} changes; learned memories "
            f"stored before Save: {review['stored_before_save']}"
        )
        out += [
            f"  - {i['change']}: {i['text']}"
            + (f" ({i['stored']})" if i['stored'] else '')
            for i in review['items']
        ]
    out += ['', 'Memories:', '']
    out += [f"- {m['text']}" for m in result['memories']]
    out += ['', '## 2. Index and search', '', stamps_line(result), '']
    for query, hits in result['searches'].items():
        shown = (
            '; '.join(
                f"{h['text']}{' (learned)' if h['learned'] else ''}"
                for h in hits
            )
            or 'nothing'
        )
        out.append(f'- `{query}`: {shown}')
    return out


def report_modify(result):
    out = [
        '## 3. Modify',
        '',
        f'Chat: {MOVE_CHAT}',
        '',
        f"Learn from chats: {result['dream']['result']}",
        '',
    ]
    for review in result['reviews']:
        out += [
            f"Review: {len(review['items'])} changes. A run while it "
            f"waits: status {review['extra']} (review pending is "
            f"{REVIEW_PENDING}). Error: {review['error']}.",
            '',
        ]
        for i, item in enumerate(review['before_save']):
            action = (
                'edited and kept'
                if str(i) in review['edited']
                else 'dropped'
                if i in review['dropped']
                else 'kept'
            )
            out.append(
                f"- [{action}] {item['change']}: {item['text']}"
                + (f" ({item['stored']})" if item['stored'] else '')
            )
        out.append('')
    stored = result['edited_stored']
    out += [
        f'Edited text stored: **{bool(stored)}**'
        + (f", previous: {stored[0]['previous']}" if stored else ''),
        '',
    ]
    out += [
        f"- changed: {c['before']} -> **{c['after']}**"
        for c in result['changed']
    ] or ['- changed: none']
    out += [f'- added: {t}' for t in result['added']]
    out += [
        '',
        stamps_line(result),
        '',
        f"`where do I live`: {hit_texts(result['where_do_i_live'])}",
    ]
    return out


def report_recall(result):
    reply = result['reply'][:600].replace('\n', '\n> ')
    return (
        ['## 4. Recall', '', f'Chat: {RECALL_CHAT}', '', '```']
        + (result['log'] or ['(no turn_memory_lookup lines)'])
        + [
            '```',
            '',
            'Meat in the reply: ' + (', '.join(result['meat_words']) or 'none'),
            '',
            f'> {reply}',
        ]
    )


def report_delete(result):
    return [
        '## 5. Delete',
        '',
        f"Deleted: {result['deleted']['text']} (clicked: "
        f"{result['clicked']}, still listed: {result['still_listed']}, "
        f"still found: {result['still_found']})",
        '',
        f"Search `what food do I eat` before: "
        f"{hit_texts(result['search_before'])}",
        '',
        f"after: {hit_texts(result['search_after'])}",
        '',
        stamps_line(result),
    ]


def write_report(run):
    results = run.state['results']
    out = [
        f"# Learned memory CDP eval: {run.chat_set['name']}",
        '',
        f'Profile `{run.profile}`.',
        '',
    ]
    for name, section in [
        ('learn', report_learn),
        ('modify', report_modify),
        ('recall', report_recall),
        ('delete', report_delete),
    ]:
        if name in results:
            out += section(results[name]) + ['']
    with open(
        os.path.join(run.run_dir, 'report.md'), 'w', encoding='utf-8'
    ) as f:
        f.write('\n'.join(out))
    with open(
        os.path.join(run.run_dir, 'report.json'), 'w', encoding='utf-8'
    ) as f:
        json.dump(run.state, f, indent=1)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n', maxsplit=1)[0]
    )
    parser.add_argument(
        '--set',
        default='life_events',
        help='a chat set of ../learned_memory_eval/chat_sets',
    )
    parser.add_argument(
        '--fresh',
        action='store_true',
        help='delete the profile and start again',
    )
    parser.add_argument('--only', nargs='*', help='run only these steps')
    parser.add_argument(
        '--profile',
        default=PROFILE_NAME,
        help='the user data dir name of the profile',
    )
    args = parser.parse_args()

    cdp.check_build()
    if args.fresh:
        shutil.rmtree(cdp.profile_path(args.profile), ignore_errors=True)
    with open(
        os.path.join(EVAL_TOOLS, 'chat_sets', f'{args.set}.json'),
        encoding='utf-8',
    ) as f:
        run = Eval(args.profile, json.load(f))

    # Loads both Ollama models for an hour, so no step waits for a cold model.
    subprocess.run(
        [sys.executable, os.path.join(EVAL_TOOLS, 'ollama_setup.py')],
        check=True,
    )
    for name, step in STEPS:
        if args.only and name not in args.only:
            continue
        if not args.only and name in run.state['done']:
            cdp.log(f'{name}: done before, skipped')
            continue
        cdp.log(f'{name}:')
        run.state['results'][name] = step(run)
        if name not in run.state['done']:
            run.state['done'].append(name)
        run.save()
        write_report(run)
    write_report(run)
    cdp.log('report:', os.path.join(run.run_dir, 'report.md'))


if __name__ == '__main__':
    main()
