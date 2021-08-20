#!/usr/bin/env python

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


CHROMIUM_POLICY_KEY = 'SOFTWARE\\\\Policies\\\\BraveSoftware\\\\Brave'


def AddBravePolicies(template_file_contents):
    highest_id = template_file_contents['highest_id_currently_used']
    policies = [
        {
            'name': 'TorDisabled',
            'type': 'main',
            'schema': {'type': 'boolean'},
            'supported_on': ['chrome.win:78-', 'chrome.mac:93-', 'chrome.linux:93-'],
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
            'desc': '''This policy allows an admin to specify that tor feature must be disabled
                    at startup.''',
        },
        {
            'name': 'IPFSEnabled',
            'type': 'main',
            'schema': {'type': 'boolean'},
            'supported_on': ['chrome.*:87-'],
            'features': {
                'dynamic_refresh': False,
                'per_profile': True,
                'can_be_recommended': False,
                'can_be_mandatory': True
            },
            'example_value': True,
            'id': 1,
            'caption': '''Enable IPFS feature''',
            'tags': [],
            'desc': '''This policy allows an admin to specify whether IPFS feature can be
                    enabled.''',
        }
    ]

    # Our new polices are added with highest id
    next_id = highest_id
    for policy in policies:
        next_id += 1
        policy['id'] = next_id
        template_file_contents['policy_definitions'].append(policy)

    # Update highest id
    template_file_contents['highest_id_currently_used'] = highest_id + len(policies)
