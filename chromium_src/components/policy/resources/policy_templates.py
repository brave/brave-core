# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import glob
import os

import brave_chromium_utils
import override_utils

# Brave's policy templates directory mirrors the upstream layout, so the
# upstream loader can be pointed at it as-is:
# `//brave/components/policy/resources/templates/policy_definitions/BraveSoftware` # pylint: disable=line-too-long
BRAVE_TEMPLATES_PATH = os.path.relpath(
    brave_chromium_utils.wspath(
        '//brave/components/policy/resources/templates'))

BRAVE_GROUP_NAME = 'BraveSoftware'


@override_utils.override_function(globals())
def _GetPoliciesAndGroups(orig_func):
    # Load Brave's policy definitions in addition to the upstream ones by
    # running the upstream loader a second time with `TEMPLATES_PATH` pointing
    # at Brave's templates directory. Brave policies must never be copied into
    # the Chromium tree: files created by a build action are unknown to the
    # build graph, which makes the generated depfile reference dependencies the
    # build system doesn't know exist.
    # The override runs in the scope of the overridden script, so its globals
    # are shared with it.
    # pylint: disable=global-statement,global-variable-undefined
    # pylint: disable=used-before-assignment
    global TEMPLATES_PATH

    result = orig_func()
    # Ignore leftovers of the old copy-into-Chromium-tree approach.
    result.pop(BRAVE_GROUP_NAME, None)

    chromium_templates_path = TEMPLATES_PATH
    TEMPLATES_PATH = BRAVE_TEMPLATES_PATH
    try:
        brave_result = orig_func()
    finally:
        TEMPLATES_PATH = chromium_templates_path

    assert BRAVE_GROUP_NAME in brave_result, (
        f"'{BRAVE_GROUP_NAME}' policies not found in {BRAVE_TEMPLATES_PATH}")
    result.update(brave_result)

    return result


@override_utils.override_function(globals())
def _LoadPolicies(orig_func):
    policies = orig_func()

    # `policies` has two notable keys:

    # 1) "policy_definitions"
    # there will be one "group" for every folder found under
    # `//components/policy/resources/templates/policy_definitions`
    # Chromium considers the folder name the group name for the policy.
    # Brave uses the group name "BraveSoftware", loaded from
    # `//brave/components/policy/resources/templates/policy_definitions`
    # by the `_GetPoliciesAndGroups` override above. The child element for the
    # group is the policy itself (those are the yaml files in the folder).
    policy_definition_yaml = policies['policy_definitions']
    assert policy_definition_yaml, "'policy_definitions' is None (did upstream change?)"  # pylint: disable=line-too-long

    brave_policies = list(
        policy_definition_yaml[BRAVE_GROUP_NAME]['policies'].keys())

    # 2) "policies"
    # This has the contents of:
    # `//components/policy/resources/templates/policies.yaml`
    # This is where we need to inject the Brave specific names. The policies
    # themselves are already defined (under `policy_definitions`), we just need
    # to add a mapping for ID (integer; unique) and name (matches name under
    # `policy_definitions`).
    policy_yaml = policies['policies']
    assert policy_yaml, "'policies' is None (did upstream change?)"

    policy_section = policy_yaml['policies']
    assert policy_section, "'policies > policies' is None (did upstream change?)"  # pylint: disable=line-too-long

    offset = max(map(int, policy_section), default=0)
    for i, entry in enumerate(brave_policies):
        policy_section[offset + i + 1] = entry

    return policies


@override_utils.override_function(globals())
def _WriteDepFile(orig_func, dep_file, target, source_files):
    # Upstream only globs its own templates directory, so list Brave's policy
    # files as dependencies too. Leftover copies in the Chromium tree (see
    # `_GetPoliciesAndGroups`) are filtered out, they are not used anymore.
    stale_path = f'/{POLICY_DEFINITIONS_KEY}/{BRAVE_GROUP_NAME}/'
    source_files = [f for f in source_files if stale_path not in f]
    brave_files = sorted(
        f.replace('\\', '/') for f in glob.glob(
            f'{BRAVE_TEMPLATES_PATH}/**/*.yaml', recursive=True))

    orig_func(dep_file, target, source_files + brave_files)
