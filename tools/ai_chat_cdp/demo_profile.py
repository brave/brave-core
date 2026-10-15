#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A demo profile for the search of conversations and for learned memory.

  ./demo_profile.py build      # make the profile from demo/leo_demo.json
  ./demo_profile.py verify     # the searches find the right chat and turn
  ./demo_profile.py rehearse   # the demo on a copy: tool, learn, one memory
  ./demo_profile.py reset      # back to the built state after a demo
  ./open_profile.py demo       # open it for the demo

The profile has the chats of demo/leo_demo.json with their dates, a full
conversation index, and no learned memory. The eval mode of the browser
imports the chats (../learned_memory_eval/run_eval.py), and the script then
removes what its Dreaming run learned. See README.md for the demo script.

Needs Ollama with clef-flash:9b and qwen3.5:9b, as learned_memory_eval.py.
"""

import argparse
import json
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import time

import cdp
import leo
import learned_memory_eval as lme
import search_profile

PROFILE_NAME = 'leo-demo'
HERE = os.path.dirname(os.path.abspath(__file__))
CHAT_SET = os.path.join(HERE, 'demo', 'leo_demo.json')
# Leo's model for the chats that are typed in the demo.
CHAT_MODEL = 'chat-automatic'
# Tables that Dreaming writes. The chats stay.
MEMORY_TABLES = ('learned_memory', 'memory_source_link', 'memory_watermark')
WINDOWS_TO_UNIX_EPOCH_S = 11644473600

# The searches of the demo: the chat and the turn that each must find. A
# turn is the entry uuid that the eval import gives: eval-<chat>-<turn>-....
SEARCHES = [
    ('air trapped in the old heaters', 'Renovating a 1920s apartment', 2),
    ('bathing rules at a Japanese hot spring', 'Japan trip in spring', 1),
    ('fungus on the tomato leaves', 'Tomato leaves with brown spots', 0),
    (
        'counting overnight hours in a spreadsheet',
        'Cleaning a messy CSV with pandas',
        1,
    ),
    ('what to eat before race day', 'Half marathon taper', 1),
    ('orange streaks on my photos', 'Shooting film again', 1),
]
# A question to Leo that makes it search the chats with its tool.
TOOL_QUESTION = 'Find my old chat about the radiators in our apartment.'
# The chat of the demo that changes one memory: Dreaming merges the new
# kitten into the memory about Mochi.
SINGLE_MEMORY_CHAT = (
    'We adopted a second cat last weekend, a rescued black kitten named '
    'Luna. How do we introduce her to Mochi?'
)
SINGLE_MEMORY_WORD = 'luna'
SINGLE_MEMORY_OLD = 'Has a Maine Coon cat named Mochi'
# A follow-up that sends the changed memory with a chat turn.
FOLLOW_UP_CHAT = (
    'We are away for a long weekend. What should the cat sitter know?'
)


def chrome_time_now():
    """A base::Time pref: microseconds since 1601, as a string."""
    return str(int((time.time() + WINDOWS_TO_UNIX_EPOCH_S) * 1_000_000))


def chat_titles():
    with open(CHAT_SET, encoding='utf-8') as f:
        return [chat['title'] for chat in json.load(f)['chats']]


def entry_prefix(title, turn):
    return f'eval-{chat_titles().index(title)}-{turn}-'


class Demo:
    """The demo profile, its built copy and the files of the tools."""

    def __init__(self, name):
        self.name = name
        self.path = cdp.profile_path(name)
        # Made with the profile (cdp.tools_dir() would make it now).
        self.files = os.path.join(self.path, 'ai_chat_cdp')
        # The state right after build, for reset. Outside the profile.
        self.built = self.path + '.built'

    def browser(self, name):
        return cdp.Browser(
            self.path,
            os.path.join(self.files, f'browser-{name}.log'),
            lme.SWITCHES,
        )


def import_chats(demo):
    """Makes the profile with the chats, in the eval mode of the browser."""
    work = tempfile.mkdtemp(prefix='leo-demo-')
    try:
        subprocess.run(
            [
                sys.executable,
                os.path.join(lme.EVAL_TOOLS, 'run_eval.py'),
                '--chat-set',
                CHAT_SET,
                '--out',
                os.path.join(work, 'run'),
                '--keep-profiles',
            ],
            check=True,
        )
        run = os.path.join(work, 'run', 'leo_demo', 'r1')
        with open(os.path.join(run, 'report.json'), encoding='utf-8') as f:
            learned = sorted(m['text'] for m in json.load(f)['memories'])
        shutil.move(os.path.join(run, 'profile'), demo.path)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return learned


def forget_learning(demo):
    """Removes what the import's Dreaming run wrote, while the browser is
    closed: the memories, their sources and the read marks."""
    db = sqlite3.connect(os.path.join(demo.path, 'Default', 'AIChat'))
    with db:
        for table in MEMORY_TABLES:
            db.execute(f'DELETE FROM {table}')
    db.close()


def set_prefs(demo):
    """The prefs that a user of the demo has: Leo's own model for new chats,
    the New Tab page at start, and the daily Dreaming one day away, so that
    only "Learn from chats" learns."""
    path = os.path.join(demo.path, 'Default', 'Preferences')
    with open(path, encoding='utf-8') as f:
        prefs = json.load(f)
    ai_chat = prefs.setdefault('brave', {}).setdefault('ai_chat', {})
    ai_chat['learned_memory_last_dreaming_time'] = chrome_time_now()
    ai_chat['default_model_key'] = CHAT_MODEL
    prefs.setdefault('session', {})['restore_on_startup'] = 5
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(prefs, f)


def index_counts(demo):
    """The chats and passages in the conversation index. The browser locks
    the database while it runs, so read it after the browser quit."""
    db = sqlite3.connect(os.path.join(demo.path, 'Default', 'AIChatEmbeddings'))
    try:
        return db.execute(
            'SELECT (SELECT COUNT(*) FROM conversation),'
            ' (SELECT COUNT(*) FROM conversation_passage)'
        ).fetchone()
    finally:
        db.close()


# The time that the browser gets to index the imported chats. 15 chats take
# about 30 s.
INDEX_TIME = 60


def build(demo):
    if os.path.exists(demo.path) or os.path.exists(demo.built):
        raise SystemExit(
            f'{demo.path} exists. Remove it and {demo.built} to build again.'
        )
    with open(CHAT_SET, encoding='utf-8') as f:
        chats = len(json.load(f)['chats'])
    cdp.log(f'importing {chats} chats; the eval run learns:')
    for text in import_chats(demo):
        cdp.log('  ', text)
    forget_learning(demo)
    set_prefs(demo)
    search_profile.enable_flag(demo)
    os.makedirs(demo.files, exist_ok=True)
    # The conversation index fills at the start.
    with demo.browser('build') as browser:
        time.sleep(INDEX_TIME)
        ws, tab = leo.open_customization(browser)
        if lme.learned(ws)['memories']:
            raise RuntimeError('the profile has learned memories')
        browser.close_tab(tab)
    chats, passages = index_counts(demo)
    cdp.log(
        f'index: {chats} of {len(chat_titles())} chats, {passages} passages'
    )
    if chats != len(chat_titles()):
        raise SystemExit('the conversation index is not full')
    keep_built(demo)
    cdp.log('built:', demo.path)


def keep_built(demo):
    """A copy of the built profile, for reset. `cp -c` clones the files on
    APFS, so the copy is fast and takes no space."""
    shutil.rmtree(demo.built, ignore_errors=True)
    if (
        subprocess.run(
            ['cp', '-c', '-R', demo.path, demo.built],
            capture_output=True,
            check=False,
        ).returncode
        != 0
    ):
        shutil.copytree(demo.path, demo.built)


def reset(demo):
    if not os.path.exists(demo.built):
        raise SystemExit(f'No {demo.built}. Run build first.')
    shutil.rmtree(demo.path, ignore_errors=True)
    subprocess.run(['cp', '-c', '-R', demo.built, demo.path], check=True)
    set_prefs(demo)
    cdp.log('reset:', demo.path)


SEARCH_RESULTS = (
    "[...document.querySelectorAll('[data-testid=conversation-search-item]')]"
    ".map(a=>({title:a.innerText.split('\\n')[0],"
    'entry:a.dataset.entryUuid||""}))'
)


def type_search(ws, query):
    ws.eval(
        f'(()=>{{const i={search_profile.SEARCH_BOX};i.focus();'
        f'i.value={json.dumps(query)};i.dispatchEvent(new '
        'InputEvent("input",{bubbles:true,composed:true}));return 1})()'
    )


def search(ws, query, tries=2):
    """Types |query| into the conversation search and gives the semantic
    results: the chat title and the entry that a click opens. A search that
    starts while the page loads finds nothing, so it is typed again."""
    for _ in range(tries):
        type_search(ws, query)
        for _ in range(20):
            time.sleep(0.5)
            cards = ws.eval(SEARCH_RESULTS)
            if cards:
                return cards
        type_search(ws, '')
        time.sleep(1)
    return []


# The turns that the frame shows from the top of the view down, and the turn
# that it highlights after a click on a search result.
FRAME_VIEW = """(()=>{const h=window.innerHeight;
  const shown=[...document.querySelectorAll('[data-entry-uuids]')]
    .filter(e=>{const r=e.getBoundingClientRect();return r.top>=-5&&r.top<h})
    .map(e=>e.getAttribute('data-entry-uuids').split(',')[0]);
  const lit=document.querySelector('[data-highlighted]');
  return {shown, highlighted: lit&&lit.getAttribute('data-entry-uuids')}})()"""


def laptop_window(ws):
    """The window of a laptop, so that a long chat does not fit in it."""
    window = ws.call('Browser.getWindowForTarget')['windowId']
    ws.call(
        'Browser.setWindowBounds',
        windowId=window,
        bounds={'width': 1280, 'height': 760, 'windowState': 'normal'},
    )


def open_first_result(ws, tab):
    """Clicks the first search result, and gives what the chat then shows."""
    ws.eval(
        "document.querySelector('[data-testid=conversation-search-item]')"
        '.click();1'
    )
    time.sleep(1.5)
    frames = [
        t
        for t in cdp.targets()
        if 'conversation-entries' in t['url'] and t.get('parentId') == tab
    ]
    return cdp.WebSocket(frames[0]['webSocketDebuggerUrl']).eval(FRAME_VIEW)


def verify(demo):
    """Every demo search finds its chat first, and a click on it opens the
    chat at that turn, in the size of a laptop window."""
    failures = 0
    with demo.browser('verify') as browser:
        ws, tab = browser.new_tab('brave://leo-ai/', settle=8)
        laptop_window(ws)
        for query, title, turn in SEARCHES:
            prefix = entry_prefix(title, turn)
            cards = search(ws, query)
            top = cards[0] if cards else {'title': None, 'entry': ''}
            ok = top['title'] == title and top['entry'].startswith(prefix)
            view = open_first_result(ws, tab) if ok else {}
            ok = ok and (view['shown'] or [''])[0].startswith(prefix)
            failures += not ok
            cdp.log(
                f'  {"ok  " if ok else "FAIL"} {query!r:44} -> '
                f'{top["title"]} ({top["entry"] or "no entry"}), shows '
                f'{(view.get("shown") or ["nothing"])[0]}'
            )
        browser.close_tab(tab)
    if failures:
        raise SystemExit(f'{failures} searches did not open their turn')


ALLOW_ONCE = """(()=>{
  const b=[...document.querySelectorAll('leo-button,button')]
    .find(e=>e.innerText.trim().endsWith('Allow once'));
  if(!b) return false; b.click(); return true})()"""
TOOL_CARDS = (
    "[...document.querySelectorAll('[data-testid=conversation-search-card]')]"
    ".map(e=>e.innerText.split('\\n')[0])"
)


def ask_tool(browser):
    """Asks Leo a question that it answers from the chats with its tool,
    allows the search once, and gives the result cards and the reply."""
    conversation = leo.Conversation(browser)
    laptop_window(conversation.ws)
    conversation.send(TOOL_QUESTION)
    time.sleep(2)
    if conversation.accept_disclaimer():
        time.sleep(2)
    allowed = False
    for _ in range(60):
        frame = conversation.frame()
        if frame and frame.eval(ALLOW_ONCE):
            allowed = True
            break
        time.sleep(1)
    cards = []
    for _ in range(120):
        time.sleep(1)
        frame = conversation.frame()
        cards = frame.eval(TOOL_CARDS) if frame else []
        if cards:
            break
    # The reply after the cards.
    leo.wait_for_reply(conversation, [TOOL_QUESTION], 0)
    frame = conversation.frame()
    cards = frame.eval(TOOL_CARDS)
    # The card opens the chat at the turn that matched.
    frame.eval(
        "document.querySelector('[data-testid=conversation-search-card]')"
        '?.click();1'
    )
    time.sleep(2)
    opened = conversation.ws.eval('location.href')
    view = conversation.frame().eval(FRAME_VIEW)
    conversation.close()
    return {
        'asked permission': allowed,
        'cards': cards,
        'card opens': opened.rsplit('/', 1)[-1],
        'shows': (view['shown'] or [None])[0],
        'highlights': view['highlighted'],
    }


# The newest Dreaming run on brave://leo-memory-internals: its header, the
# chat turns that it read, and the model requests that it shows.
DREAMING_RUN = """(()=>({
  header: document.querySelector('.run-header h2')?.innerText,
  turns: [...document.querySelectorAll('.turn h3')].map(e=>e.innerText),
  decisions: document.querySelectorAll('details.call summary').length,
  llm: [...document.querySelectorAll('details.call summary')]
    .filter(e=>e.innerText.startsWith('LLM')).length}))()"""
# The memories that the frame lists under the last reply.
MEMORIES_SENT = """(()=>{
  const c=[...document.querySelectorAll('leo-collapse')].pop();
  if(!c) return null;
  return c.textContent})()"""


def internals_tab(browser, tab_index):
    ws, _ = browser.new_tab('brave://leo-memory-internals', settle=4)
    ws.eval(
        f"[...document.querySelectorAll('.tabs button')][{tab_index}].click();1"
    )
    time.sleep(3)
    return ws


def learn_all(browser, problems):
    """Step 3: Learn from chats finds the seven facts and none of the
    traps, and the user keeps them all."""
    ws, tab = leo.open_customization(browser)
    lme.dream_now(ws)
    items = lme.open_review(ws)['items']
    lme.handle_review(ws, lambda items: (set(), {}))
    memories = lme.learned(ws)['memories']
    browser.close_tab(tab)
    cdp.log('learn from chats, review:')
    for item in items:
        cdp.log('  ', item['change'], item['text'])
    with open(CHAT_SET, encoding='utf-8') as f:
        expect = dict(json.load(f)['expect'])
    # The chat set names the forbidden words alone (README.md of
    # ../learned_memory_eval), and check_expectations() takes rules.
    expect['forbidden'] = [
        {'label': f'no {word}', 'all': [word]}
        for word in expect.get('forbidden', [])
    ]
    for kind, label, ok, hit in lme.check_expectations(memories, expect):
        if not ok:
            problems.append(f'step 3: {kind} {label}: {hit}')
    if any(item['change'] != 'New' for item in items):
        problems.append('step 3: a change of the review is not new')


def learn_one(browser, problems):
    """Step 4: one new chat, then Learn from chats: one change of the
    memory about Mochi, and the run on the internals page with the chat and
    its model requests."""
    leo.chat(browser, [SINGLE_MEMORY_CHAT])
    ws, tab = leo.open_customization(browser)
    lme.dream_now(ws)
    items = lme.open_review(ws)['items']
    lme.handle_review(ws, lambda items: (set(), {}))
    browser.close_tab(tab)
    cdp.log(
        'one new chat, review:',
        [(i['change'], i['text'], i['stored']) for i in items],
    )
    if not (
        len(items) == 1
        and items[0]['change'] == 'Changed'
        and (items[0]['stored'] or '').endswith(SINGLE_MEMORY_OLD)
        and SINGLE_MEMORY_WORD in items[0]['text'].lower()
    ):
        problems.append(f'step 4: the review is not one change: {items}')
    run = internals_tab(browser, 0).eval(DREAMING_RUN)
    cdp.log('dreaming run:', run)
    read = [t for t in run['turns'] if t.startswith('Turn: ')]
    if ' dream_now ' not in (run['header'] or ''):
        problems.append(f'step 4: the newest run is {run["header"]}')
    if not any(SINGLE_MEMORY_WORD in t.lower() for t in read):
        problems.append(f'step 4: the run did not read the chat: {read}')
    if not run['decisions'] or not run['llm']:
        problems.append('step 4: the run shows no model request')


def use_memory(browser, problems):
    """Step 5: a follow-up sends the new memory; the reply lists it, and the
    Chat turns tab shows the lookup."""
    conversation = leo.Conversation(browser)
    conversation.send(FOLLOW_UP_CHAT)
    time.sleep(2)
    if conversation.accept_disclaimer():
        time.sleep(2)
    leo.wait_for_reply(conversation, [FOLLOW_UP_CHAT], 0)
    sent = conversation.frame().eval(MEMORIES_SENT) or ''
    conversation.close()
    done = json.loads(
        internals_tab(browser, 1).eval(
            "document.querySelector('.step-done pre').innerText"
        )
    )
    chosen = done['relevant'] + done['permanent']
    cdp.log('follow-up, the lookup sends:', chosen)
    cdp.log('follow-up, under the reply:', ' | '.join(sent.splitlines()))
    if not any(SINGLE_MEMORY_WORD in m.lower() for m in chosen):
        problems.append(f'step 5: the lookup did not send Luna: {chosen}')
    if SINGLE_MEMORY_WORD not in sent.lower():
        problems.append('step 5: the list under the reply has no Luna')


def rehearse(demo):
    """Runs demo steps 2 to 5 of README.md on a copy of the profile, and
    names every step that gives something else than the README says."""
    copy = Demo(demo.name + '-rehearsal')
    shutil.rmtree(copy.path, ignore_errors=True)
    subprocess.run(['cp', '-c', '-R', demo.built, copy.path], check=True)
    set_prefs(copy)
    problems = []
    try:
        with copy.browser('rehearse') as browser:
            tool = ask_tool(browser)
            cdp.log('conversation search tool:', tool)
            if not tool['cards'] or tool['cards'][0] != SEARCHES[0][1]:
                problems.append(f'step 2: the cards are {tool["cards"]}')
            if not (tool['highlights'] or '').startswith(
                entry_prefix(SEARCHES[0][1], 2)
            ):
                problems.append('step 2: the card did not open the turn')
            learn_all(browser, problems)
            learn_one(browser, problems)
            use_memory(browser, problems)
    finally:
        shutil.rmtree(copy.path, ignore_errors=True)
    for problem in problems:
        cdp.log('FAIL', problem)
    if problems:
        raise SystemExit(f'{len(problems)} demo steps went wrong')
    cdp.log('all demo steps as in README.md')


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n', maxsplit=1)[0]
    )
    parser.add_argument(
        'command', choices=['build', 'verify', 'rehearse', 'reset']
    )
    parser.add_argument(
        '--profile',
        default=PROFILE_NAME,
        help='the user data dir name of the profile',
    )
    args = parser.parse_args()

    cdp.check_build()
    demo = Demo(args.profile)
    if args.command == 'build':
        build(demo)
    elif args.command == 'verify':
        verify(demo)
    elif args.command == 'rehearse':
        subprocess.run(
            [sys.executable, os.path.join(lme.EVAL_TOOLS, 'ollama_setup.py')],
            check=True,
        )
        rehearse(demo)
    else:
        reset(demo)


if __name__ == '__main__':
    main()
