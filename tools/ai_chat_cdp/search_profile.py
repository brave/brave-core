#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A test profile for the semantic search of Leo's conversations and memories.

  ./search_profile.py setup    # turn the features on, copy the embedder
  ./search_profile.py seed     # type the chats into Leo, add the memories
  ./search_profile.py verify   # run the searches, print what they find

Everything goes through the UI over CDP. The chats are answered by Leo's
default model. `seed` can run again: it does only what is left.

Open the profile by hand with:
  pnpm start --user_data_dir_name=leo-search-test
"""

import argparse
import json
import os
import shutil
import time

import cdp
import leo

PROFILE_NAME = 'leo-search-test'

# Each conversation is about one topic. The Japan trip is long on purpose, so
# that opening it at a matching entry has something to scroll through.
CONVERSATIONS = {
    'sourdough': [
        'How often should I feed a sourdough starter that lives in the '
        'fridge, and how do I revive it before baking a loaf? Answer in '
        'two or three short paragraphs.',
    ],
    'wireguard': [
        'How do I set up a WireGuard VPN server on a Raspberry Pi so my '
        'phone can reach my home network? Answer in two or three short '
        'paragraphs.',
    ],
    'cat_allergy': [
        'Can you talk about cat allergy? Cover the allergen, the symptoms, '
        'the treatment and how to reduce exposure at home.',
    ],
    'bicycle': [
        'Write four paragraphs about the history of the bicycle.',
    ],
    'hepa': [
        'Do HEPA air purifiers help with pet dander? Answer in two or '
        'three short paragraphs.',
    ],
    'meta_muse': [
        "What is the feedback on Meta's Muse personal AI agent? Give a "
        'short rundown of what works and what falls short.',
    ],
    # The detail to search for, the deer in Nara Park, is in the third turn.
    'japan_trip': [
        "I'm planning a week in Japan. Write four paragraphs on a sensible "
        'itinerary for Tokyo and Kyoto.',
        'Write four paragraphs about what to eat in Tokyo and Kyoto.',
        'Is a day trip from Kyoto to Nara worth it? Write four paragraphs, '
        'including the deer in Nara Park and how to get there.',
        'Write four paragraphs about how to use trains and the JR Pass.',
        'Write four paragraphs about a daily budget and what to pack.',
    ],
}

MEMORIES = [
    'say hello to your dog and cat',
    'muse agent is so bad that it would ruin your life',
    'I am allergic to peanuts',
    'My favorite programming language is Rust',
    'I live in Vancouver and commute by bike',
]

CONVERSATION_QUERIES = [
    'deer in Nara Park',
    'keeping a bread dough culture alive',
    'remote access to my home network',
    'sneezing around pets',
    "reviews of Meta's AI agent",
]
MEMORY_QUERIES = [
    'pets at home',
    'food allergies',
    'what language do I code in',
    'how do I get to work',
]

# Turns on the Semantic history search feature, as brave://flags does.
HISTORY_EMBEDDINGS_FLAG = 'brave-history-embeddings@1'

SEARCH_BOX = (
    f'(()=>{{const f={cdp.FIND};const h=f(e=>e.tagName===\'LEO-INPUT\'&&'
    "(e.getAttribute('placeholder')||'').length>0);"
    "return h.shadowRoot.querySelector('input')})()"
)
CONVERSATION_TITLES = (
    "[...document.querySelectorAll('[data-testid=conversation-title]')]"
    '.map(e=>e.textContent.trim())'
)
SEARCH_CARDS = (
    "[...document.querySelectorAll('[data-testid=conversation-search-item]')]"
    ".map(a=>a.innerText.split('\\n')[0])"
)


class Profile:
    """The test profile and the files of the tools in it."""

    def __init__(self, name):
        self.path = cdp.profile_path(name)
        self.files = cdp.tools_dir(self.path)
        self.state_path = os.path.join(self.files, 'search_profile.json')

    def browser(self, name):
        return cdp.Browser(
            self.path, os.path.join(self.files, f'browser-{name}.log')
        )

    def load_state(self):
        if os.path.exists(self.state_path):
            with open(self.state_path, encoding='utf-8') as f:
                return json.load(f)
        return {'conversations': [], 'memories': False}

    def save_state(self, state):
        with open(self.state_path, 'w', encoding='utf-8') as f:
            json.dump(state, f, indent=1)


def copy_embedder(profile):
    """The embedder model comes from the dev profile, so it is not
    downloaded again."""
    target = os.path.join(profile.path, 'BraveLocalAIModels')
    source = os.path.join(cdp.dev_profile_path(), 'BraveLocalAIModels')
    if os.path.exists(target):
        return
    if not os.path.exists(source):
        raise SystemExit(
            f'No embedder in {source}. Start the dev build once '
            'with Semantic history search on, and wait for it.'
        )
    shutil.copytree(source, target)


def enable_flag(profile):
    """Sets the flag in Local State while the browser is closed."""
    local_state = os.path.join(profile.path, 'Local State')
    state = {}
    if os.path.exists(local_state):
        with open(local_state, encoding='utf-8') as f:
            state = json.load(f)
    labs = state.setdefault('browser', {}).setdefault(
        'enabled_labs_experiments', []
    )
    if HISTORY_EMBEDDINGS_FLAG not in labs:
        labs.append(HISTORY_EMBEDDINGS_FLAG)
    with open(local_state, 'w', encoding='utf-8') as f:
        json.dump(state, f)


def setup(profile):
    os.makedirs(profile.path, exist_ok=True)
    copy_embedder(profile)
    # A first start creates the profile.
    with profile.browser('setup'):
        time.sleep(5)
    enable_flag(profile)
    with profile.browser('setup') as browser:
        for key, value in [
            ('brave.history_embeddings_enabled', True),
            ('brave.ai_chat.storage_enabled', True),
            ('brave.ai_chat.user_memory_enabled', True),
            # Open the New Tab page, not the last session.
            ('session.restore_on_startup', 5),
        ]:
            if not browser.set_pref(key, value):
                raise RuntimeError(f'could not set {key}')
    # Semantic history search takes effect at the next start.
    with profile.browser('setup') as browser:
        cdp.log(
            browser.get_prefs(
                r'history_embeddings_enabled|ai_chat\.(storage|user_memory)'
            )
        )
    cdp.log('set up:', profile.path)


def seed(profile, only):
    state = profile.load_state()
    todo = [
        name
        for name in CONVERSATIONS
        if (not only or name in only) and name not in state['conversations']
    ]
    cdp.log(
        'to seed:',
        todo or 'no chat',
        '| memories:',
        'done' if state['memories'] else 'to do',
    )
    with profile.browser('seed') as browser:
        if not state['memories'] and not only:
            ws, tab = leo.open_customization(browser)
            existing = leo.memories(ws)
            for memory in MEMORIES:
                if memory not in existing:
                    leo.add_memory(ws, memory)
                    time.sleep(0.5)
            time.sleep(2)
            cdp.log('memories:', leo.memories(ws))
            browser.close_tab(tab)
            state['memories'] = True
            profile.save_state(state)
        for name in todo:
            cdp.log('chat:', name)
            leo.chat(browser, CONVERSATIONS[name])
            state['conversations'].append(name)
            profile.save_state(state)
        # The last entries reach the index.
        time.sleep(40)


def verify(profile):
    """Prints what the searches find. Leo can save a memory by itself during
    the chats; such memories go first, so the memory list is the data set."""
    with profile.browser('verify') as browser:
        ws, tab = leo.open_customization(browser)
        extra = [m for m in leo.memories(ws) if m not in MEMORIES]
        for memory in extra:
            leo.delete_memory(ws, memory)
        if extra:
            cdp.log('deleted the memories that Leo saved itself:', extra)
            time.sleep(10)  # the index follows
        cdp.log('memories:', leo.memories(ws))
        cdp.log('memory search:')
        for query in MEMORY_QUERIES:
            found = [hit['text'] for hit in leo.search_memories(ws, query)]
            cdp.log(f'  {query!r:42} -> {found}')
        browser.close_tab(tab)

        ws, tab = browser.new_tab('brave://leo-ai/', settle=8)
        cdp.log('conversations:', ws.eval(CONVERSATION_TITLES))
        cdp.log('conversation list search:')
        for query in CONVERSATION_QUERIES:
            ws.eval(
                f'(()=>{{const i={SEARCH_BOX};i.focus();'
                f'i.value={json.dumps(query)};i.dispatchEvent(new '
                'InputEvent("input",{bubbles:true,composed:true}));'
                'return 1})()'
            )
            cards = []
            for _ in range(20):
                time.sleep(0.5)
                cards = ws.eval(SEARCH_CARDS)
                if cards:
                    break
            cdp.log(f'  {query!r:42} -> {cards}')
        browser.close_tab(tab)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n', maxsplit=1)[0]
    )
    parser.add_argument('command', choices=['setup', 'seed', 'verify'])
    parser.add_argument(
        'chats', nargs='*', help='seed only these chats, and no memory'
    )
    parser.add_argument(
        '--profile',
        default=PROFILE_NAME,
        help='the user data dir name of the profile',
    )
    args = parser.parse_args()

    cdp.check_build()
    profile = Profile(args.profile)
    if args.command == 'setup':
        setup(profile)
    elif args.command == 'seed':
        seed(profile, args.chats)
    else:
        verify(profile)


if __name__ == '__main__':
    main()
