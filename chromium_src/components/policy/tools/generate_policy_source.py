# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils

CHROMIUM_POLICY_KEY = 'SOFTWARE\\\\Policies\\\\BraveSoftware\\\\Brave'

@override_utils.override_function(globals())
def _LoadJSONFile(orig_func, json_file):
    json = orig_func(json_file)
    AddBravePolicies(json)
    return json

def AddBravePolicies(template_file_contents):
    highest_id = template_file_contents['highest_id_currently_used']
    policies = [
        {
            'name': 'TorDisabled',
            'type': 'main',
            'schema': {
                'type': 'boolean'
            },
            'supported_on': [
                'chrome.win:78-', 'chrome.mac:93-', 'chrome.linux:93-'
            ],
            'features': {
                'dynamic_refresh': False,
                'per_profile': False,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': True,
            'id': 0,
            'caption': '''Disables the tor feature.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that tor '''
                     '''must be disabled at startup.'''),
        },
        {
            'name': 'BraveRewardsDisabled',
            'type': 'main',
            'schema': {
                'type': 'boolean'
            },
            'supported_on': ['chrome.*:105-'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': True,
            'id': 2,
            'caption': '''Disable Brave Rewards feature.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that Brave '''
                     '''Rewards feature will be disabled.'''),
        },
        {
            'name': 'BraveWalletDisabled',
            'type': 'main',
            'schema': {
                'type': 'boolean'
            },
            'supported_on': ['chrome.*:106-'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': True,
            'id': 3,
            'caption': '''Disable Brave Wallet feature.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that Brave '''
                     '''Wallet feature will be disabled.'''),
        },
        {
            'name': 'BraveShieldsDisabledForUrls',
            'type': 'main',
            'schema': {
                'type': 'array',
                'items': {
                    'type': 'string'
                },
            },
            'supported_on': ['chrome.*:107-'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': ['https://brave.com'],
            'id': 4,
            'caption': '''Disables Brave Shields for urls.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that Brave '''
                     '''Shields disabled.'''),
        },
        {
            'name': 'BraveShieldsEnabledForUrls',
            'type': 'main',
            'schema': {
                'type': 'array',
                'items': {
                    'type': 'string'
                },
            },
            'supported_on': ['chrome.*:107-'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': ['https://brave.com'],
            'id': 5,
            'caption': '''Enables Brave Shields for urls.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that Brave '''
                     '''Shields enabled.'''),
        },
        {
            'name': 'BraveVPNDisabled',
            'type': 'main',
            'schema': {
                'type': 'boolean'
            },
            'supported_on': ['chrome.*:112-'],
            'future_on': ['android'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': True,
            'id': 6,
            'caption': '''Disable Brave VPN feature.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that Brave '''
                     '''VPN feature will be disabled.'''),
        },
        {
            'name': 'BraveAIChatEnabled',
            'type': 'main',
            'schema': {
                'type': 'boolean'
            },
            'supported_on': ['chrome.*:121-'],
            'future_on': ['android'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': True,
            'id': 7,
            'caption': '''Enable Brave AI Chat feature.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify that Brave '''
                     '''AI Chat feature will be enabled.'''),
        },
        {
            'name': 'BraveSyncUrl',
            'type': 'main',
            'schema': {
                'type': 'string'
            },
            'supported_on': ['chrome.*:129-'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': ['https://sync-v2.brave.com/v2'],
            'id': 8,
            'caption': '''Custom sync server URL.''',
            'tags': [],
            'desc': ('''This policy allows an admin to specify a custom '''
                     '''sync server URL for Brave.'''),
        },
    ]

    # Our new polices are added with highest id
    next_id = highest_id + 1
    for policy in policies:
        next_id += 1
        policy['id'] = next_id
        template_file_contents['policy_definitions'].append(policy)

    # Update highest id
    template_file_contents['highest_id_currently_used'] = highest_id + \
        len(policies)
