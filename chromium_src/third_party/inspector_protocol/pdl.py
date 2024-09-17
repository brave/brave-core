# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import re
import pprint

import brave_chromium_utils
import override_utils

_original_parse = parse


def _merge_types(extended_domain, protocol_domain):
    extended_types = extended_domain.get('types', None)
    if not extended_types:
        return
    protocol_types = protocol_domain.get('types', None)
    if not protocol_types:
        protocol_domain['types'] = extended_types
        return
    for extended_type in extended_types:
        protocol_type = next(
            filter(lambda x: x['id'] == extended_type['id'], protocol_types),
            None)
        if not protocol_type:
            protocol_types.append(extended_type)
            continue
        merged = False
        extended_enum = extended_type.get('enum', None)
        if extended_enum:
            protocol_type['enum'].extend(extended_enum)
            merged = True
        if not merged:
            # Please add new PDL merging capabilities if you need to.
            raise RuntimeError("Unsupported type merge: ", extended_type)


def _merge_commands(extended_domain, protocol_domain):
    extended_commands = extended_domain.get('commands', None)
    if not extended_commands:
        return
    protocol_commands = protocol_domain.get('commands', None)
    if not protocol_commands:
        protocol_domain['commands'] = extended_commands
        return
    for extended_command in extended_commands:
        protocol_command = next(
            filter(lambda x: x['name'] == extended_command['name'],
                   protocol_commands), None)
        if not protocol_command:
            protocol_commands.append(extended_command)
            continue
        # Please add new PDL merging capabilities if you need to.
        raise RuntimeError("Unsupported command merge: ", extended_command)


def _merge_events(extended_domain, protocol_domain):
    extended_events = extended_domain.get('events', None)
    if not extended_events:
        return
    protocol_events = protocol_domain.get('events', None)
    if not protocol_events:
        protocol_domain['events'] = extended_events
        return
    for extended_event in extended_events:
        protocol_event = next(
            filter(lambda x: x['name'] == extended_event['name'],
                   protocol_events), None)
        if not protocol_event:
            protocol_events.append(extended_event)
            continue
        # Please add new PDL merging capabilities if you need to.
        raise RuntimeError("Unsupported event merge: ", extended_event)


def _merge_protocol(protocol, file_name):
    chromium_src_file = brave_chromium_utils.get_chromium_src_override(
        file_name)
    if not os.path.exists(chromium_src_file):
        return
    with open(chromium_src_file, "r") as input_file:
        extended_protocol = _original_parse(input_file.read(),
                                            chromium_src_file)

    for extended_domain in extended_protocol['domains']:
        protocol_domain = None
        for domain in protocol['domains']:
            if domain['domain'] == extended_domain['domain']:
                protocol_domain = domain
                break

        if protocol_domain:
            _merge_types(extended_domain, protocol_domain)
            _merge_commands(extended_domain, protocol_domain)
            _merge_events(extended_domain, protocol_domain)
        else:
            assert extended_domain['domain'].startswith('Brave')
            protocol['domains'].append(extended_domain)

@override_utils.override_function(globals())
def parse(original_function, data, file_name, map_binary_to_string=False):
    protocol = original_function(data, file_name, map_binary_to_string)
    _merge_protocol(protocol, file_name)
    return protocol
