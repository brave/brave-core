# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import hashlib
import json
import os
import override_utils
import shutil

from brave_chromium_utils import wspath


@override_utils.override_function(globals())
def _LoadPolicies(orig_func):
    policies = orig_func()

    # `policies` has two notable keys:

    # 1) "policy_definitions"
    # there will be one "group" for every folder found under
    # `//components/policy/resources/templates/policy_definitions`
    # Chromium considers the folder name the group name for the policy.
    # Brave uses the group name "BraveSoftware". The child element for the
    # group is the policy itself (those are the yaml files in the folder).
    #
    # Brave specific entries are get copied into place by `update_policy_files`.
    # We copy the files from:
    # `//brave/components/policy/resources/templates/policy_definitions/BraveSoftware` # pylint: disable=line-too-long
    # to:
    # `//components/policy/resources/templates/policy_definitions`
    policy_definition_yaml = policies['policy_definitions']
    assert policy_definition_yaml, "'policy_definitions' is None (did upstream change?)"  # pylint: disable=line-too-long

    brave_policies = []
    brave_policy_section = policy_definition_yaml['BraveSoftware']
    assert brave_policy_section, "'policy_definitions > BraveSoftware' entries not found (failed to copy?)"  # pylint: disable=line-too-long

    brave_policy_items = brave_policy_section['policies']
    for key, _ in brave_policy_items.items():
        brave_policies.append(key)

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


def update_policy_files():
    # Chromium stores all group policy definitions under
    # `//components/policy/resources/templates/policy_definitions/`
    #
    # The name of the file (minus the extension; ex: TorDisable.yaml would be
    # TorDisable) corresponds to an auto-generated entry in:
    # `//out/<build_type_here>/gen/chrome/app/policy/policy_templates.json
    #
    # That auto-generated value (ex: `policy::key::kTorDisabled`) is referenced
    # when we map to a preference in our policy map:
    # `//brave/browser/policy/brave_simple_policy_map.h`
    #
    # When the code below is ran this will copy the group policy files from:
    # `//brave/components/policy/resources/templates/policy_definitions/`
    # to their expected place in Chromium:
    # `//components/policy/resources/templates/policy_definitions/`
    #
    # NOTE: only the `BraveSoftware` folder is copied.
    # If you want to create a policy in an existing Chromium group, this
    # would need to be updated.
    shutil.copytree(
        wspath(
            "//brave/components/policy/resources/templates/policy_definitions/BraveSoftware"  # pylint: disable=line-too-long
        ),
        wspath(
            "//components/policy/resources/templates/policy_definitions/BraveSoftware"  # pylint: disable=line-too-long
        ),
        dirs_exist_ok=True,
        copy_function=copy_only_if_modified)


def copy_only_if_modified(src, dst):
    """Copy file if it doesn't exist or if its hash is different."""

    def file_hash(file_path):
        with open(file_path, "rb") as f:
            return hashlib.file_digest(f, "sha256").digest()

    if not os.path.exists(dst) or file_hash(src) != file_hash(dst):
        shutil.copy2(src, dst)


@override_utils.override_function(globals())
def main(orig_func):
    update_policy_files()
    orig_func()
