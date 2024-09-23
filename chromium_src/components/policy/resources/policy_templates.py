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

    # `policies` will have the following notable keys:
    #
    # "policy_definitions"
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
    #
    #
    # "policies"
    # This has the contents of:
    # `//components/policy/resources/templates/policies.yaml`
    # This is where we need to inject the Brave specific names. The policies
    # themselves are already defined (under `policy_definitions`), we just need
    # to add a mapping for ID (integer; unique) and name (matches name under
    # `policy_definitions`).
    #
    #
    # There are some other fields which are not used by this script.

    policy_yaml = policies['policies']
    policy_section = policy_yaml['policies']

    # get the highest ID in the file
    highest_number = 0
    for key, _ in policy_section.items():
        if int(key) > highest_number:
            highest_number = int(key)

    # append our entries to the ones from policies.yaml
    # TODO(bsclifton): we can create this array dynamically by reading the file
    # names from:
    # `//brave/components/policy/resources/templates/policy_definitions/BraveSoftware` # pylint: disable=line-too-long
    brave_policies = [
        'TorDisabled', 'BraveRewardsDisabled', 'BraveWalletDisabled',
        'BraveVPNDisabled', 'BraveAIChatEnabled', 'BraveSyncUrl',
        'BraveShieldsDisabledForUrls', 'BraveShieldsEnabledForUrls'
    ]
    for entry in brave_policies:
        highest_number += 1
        #policy_key = str(highest_number)
        policy_section[highest_number] = entry

    return policies


def update_policy_files():
    # Chromium stores all group policy definitions under
    # `//components/policy/resources/templates/policy_definitions/`
    #
    # The name of the file (minus the extension; ex: TorDisable.yaml => TorDisable)
    # corresponds to an auto-generated entry in:
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
    policy_dir = wspath(
        "//brave/components/policy/resources/templates/policy_definitions/")
    with os.scandir(policy_dir) as entries:
        for entry in entries:
            if not entry.is_dir():
                continue
            src_dir = entry.path
            src_dir_name = entry.name
            dst_dir = wspath(
                f"//components/policy/resources/templates/policy_definitions/{src_dir_name}"  # pylint: disable=line-too-long
            )
            shutil.copytree(src_dir,
                            dst_dir,
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
