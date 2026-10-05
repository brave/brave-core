# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit checks for brave/browser.

See docs/best-practices/architecture.md (ARCH-074) for where tab-scoped
features belong.
"""

import chromium_presubmit_overrides

PRESUBMIT_VERSION = '2.0.0'

_BRAVE_TAB_FEATURES_CC = 'brave_tab_features.cc'
_BRAVE_TAB_HELPERS_CC = 'brave_tab_helpers.cc'

# Matches a SupportsUserData-style creation call, capturing the class.
_CREATE_FOR_CALL = r'([\w:]+)::(\w*CreateFor\w*|CreateIfNeeded)\('

# Calls BraveTabFeatures may make although they contain "CreateFor".
#
# Each must be a helper that also has to attach to WebContents that are not
# tabs. Justify additions in the change description.
_ALLOWED_CREATE_FOR_CALLS = ()

# Helpers brave::AttachTabHelpers() may attach to the WebContents.
#
# Each must also be attachable to WebContents that are not tabs, so it cannot
# be owned by BraveTabFeatures. A helper that is only ever tab-scoped belongs
# in BraveTabFeatures instead. Justify additions in the change description.
_ALLOWED_WEB_CONTENTS_HELPERS = (
    # Attached by brave::AttachPrivacySensitiveTabHelpers(), which AI chat also
    # calls for the WebContents it creates for associated content.
    'content_settings::PageSpecificContentSettings',
    'brave_shields::BraveShieldsWebContentsObserver',
    'ephemeral_storage::EphemeralStorageTabHelper',
)


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


def CheckTests(input_api, output_api):
    """Runs PRESUBMIT_test.py when this presubmit changes."""
    if not any(
        input_api.os_path.basename(f.LocalPath()).startswith('PRESUBMIT')
        for f in input_api.AffectedFiles()
    ):
        return []
    test_path = input_api.os_path.join(
        input_api.PresubmitLocalPath(), 'PRESUBMIT_test.py'
    )
    return input_api.RunTests(
        [
            input_api.Command(
                name=test_path,
                cmd=[input_api.python3_executable, test_path],
                kwargs={'cwd': input_api.PresubmitLocalPath()},
                message=output_api.PresubmitError,
            )
        ]
    )


def CheckNoCreateForInBraveTabFeatures(input_api, output_api):
    """Prevents SupportsUserData-style CreateFor* calls in BraveTabFeatures.

    Brave's counterpart of CheckNoCreateForInTabFeatures in
    chrome/browser/ui/tabs/PRESUBMIT.py: tab-scoped features must be owned by
    BraveTabFeatures as std::unique_ptr members.
    """
    results = []
    for f in input_api.AffectedFiles(include_deletes=False):
        if input_api.os_path.basename(f.LocalPath()) != _BRAVE_TAB_FEATURES_CC:
            continue
        for line_num, line in enumerate(f.NewContents(), start=1):
            if 'CreateFor' not in line:
                continue
            if any(allowed in line for allowed in _ALLOWED_CREATE_FOR_CALLS):
                continue
            results.append(
                output_api.PresubmitError(
                    f'{f.LocalPath()}:{line_num}: "CreateFor" indicates the '
                    'SupportsUserData anti-pattern. Tab-scoped features must '
                    'be owned by BraveTabFeatures as a std::unique_ptr member '
                    'instead. If the call attaches nothing to the WebContents, '
                    'add it to _ALLOWED_CREATE_FOR_CALLS in '
                    'browser/PRESUBMIT.py with justification.'
                )
            )
    return results


def CheckNoNewTabHelpersInBraveTabHelpers(input_api, output_api):
    """Keeps brave::AttachTabHelpers() remove-only for tab-scoped helpers.

    A helper may be added only if it also has to attach to WebContents that
    are not tabs. Helpers already in the file are not flagged, so lines can be
    moved or reformatted.
    """
    create_for = input_api.re.compile(_CREATE_FOR_CALL)
    results = []
    for f in input_api.AffectedFiles(include_deletes=False):
        if input_api.os_path.basename(f.LocalPath()) != _BRAVE_TAB_HELPERS_CC:
            continue
        existing = {
            m.group(1)
            for line in f.OldContents()
            for m in create_for.finditer(line)
        }
        for line_num, line in f.ChangedContents():
            for m in create_for.finditer(line):
                helper = m.group(1)
                if (
                    helper in existing
                    or helper in _ALLOWED_WEB_CONTENTS_HELPERS
                ):
                    continue
                results.append(
                    output_api.PresubmitError(
                        f'{f.LocalPath()}:{line_num}: {helper} is attached to '
                        'the WebContents. Tab-scoped features must be owned by '
                        'BraveTabFeatures instead (see ARCH-074 in '
                        'docs/best-practices/architecture.md). If the helper '
                        'must also attach to WebContents that are not tabs, '
                        'add it to _ALLOWED_WEB_CONTENTS_HELPERS in '
                        'browser/PRESUBMIT.py with justification.'
                    )
                )
    return results
