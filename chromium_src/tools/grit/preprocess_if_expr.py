# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import override_utils
import os
import shutil
import json

from brave_chromium_utils import get_chromium_src_override, get_webui_overriden_file_name, get_webui_overridden_but_referenced_files, get_src_dir, wspath


def purge_overrides(out_folder, in_files):
    """Deletes all overridden upstream files from the preprocess directory - we
       need this so removing an override doesn't break the build."""
    for file in get_webui_overridden_but_referenced_files(
            out_folder, in_files):
        os.remove(os.path.join(out_folder, file))


def maybe_keep_upstream_version(override_in_folder, out_folder, override_file):
    """Decides whether we should keep the upstream version of a file by looking
       at the override and see if it references the upstream version.
       Returns the path to the upstream file, if we keep it."""
    overridden_name = get_webui_overriden_file_name(override_file)

    # The path to the brave-core override file
    override_in_path = os.path.join(override_in_folder, override_file)

    # The path to the upstream file
    upstream_file_path = os.path.join(out_folder, override_file)

    with open(override_in_path) as f:
        text = f.read()

        # If we reference the upstream file in our overridden file, make sure
        # we keep it around.
        if os.path.basename(overridden_name).replace('.ts', '.js') in text:
            shutil.copy(upstream_file_path,
                        os.path.join(out_folder, overridden_name))
            return overridden_name
    return None


def run_mangler(out_folder, mangler_file, preprocess_file):
    """Runs the mangler on the given file"""
    import brave_node

    tsx = brave_node.PathInNodeModules('tsx', 'dist', 'cli.mjs')
    ts_config = wspath("//brave/tsconfig-mangle.json")
    lit_mangler = wspath(
        "//brave/tools/chromium_src/lit_mangler/lit_mangler_cli.ts")

    # Note: We read from and write to the preprocess file - this way any
    # preprocessing that upstream does will be mangled.
    brave_node.RunNode([
        tsx, '--tsconfig', ts_config, lit_mangler, 'mangle', '--typecheck',
        '-m', mangler_file, '-i', preprocess_file, '-o', preprocess_file, '-g',
        out_folder
    ])


def get_chromium_src_files(in_folder, in_files):
    """Gets all the overrides we have in brave-core for this target"""
    override_files = []
    lit_mangler_files = []

    for file in in_files:
        override_file = get_chromium_src_override(os.path.join(
            in_folder, file))
        if os.path.exists(override_file):
            if should_run_mangler(override_file):
                lit_mangler_files.append((file, override_file))
            else:
                override_files.append(file)

    return override_files, lit_mangler_files


def should_run_mangler(in_file):
    """Determines if we should run the mangler on the given file"""
    return in_file.endswith('.lit_mangler.ts')


# Used to indicate that there are no overrides, so we don't need to reprocess
class NoOverridesException(Exception):
    pass


@override_utils.override_function(globals())
def main(original_function, argv):
    # Run the preprocessor as normal, to preprocess the upstream files.
    original_function(argv)

    manifest_data = {}
    manifest_path = None

    # When we rerun the processor, we detect files which should be overridden.
    @override_utils.override_method(argparse.ArgumentParser)
    # pylint: disable=unused-variable
    def parse_args(self, original_method, argv):
        nonlocal manifest_path, manifest_data
        args = original_method(self, argv)

        cwd = os.getcwd()
        out_folder = os.path.normpath(os.path.join(cwd, args.out_folder))
        in_folder = os.path.normpath(os.path.join(cwd, args.in_folder))
        override_root_folder = get_chromium_src_override(in_folder)

        purge_overrides(out_folder, args.in_files)
        overrides, lit_mangler_files = get_chromium_src_files(
            in_folder, args.in_files)

        # Run the manglers - this doesn't need to happen in the second call to main because we don't depend on anything there.
        for upstream_file, lit_mangler_file in lit_mangler_files:
            run_mangler(out_folder,
                        os.path.join(override_root_folder, lit_mangler_file),
                        os.path.join(out_folder, upstream_file))

        if len(overrides) == 0:
            # Throw an exception to abort the second call to `main()` early if no overrides were found.
            raise NoOverridesException()

        if args.out_manifest:
            manifest_path = os.path.join(cwd, args.out_manifest)
            manifest_data = json.load(open(manifest_path))

        for override_file in overrides:
            overridden_name = maybe_keep_upstream_version(
                override_root_folder, out_folder, override_file)
            if overridden_name and args.out_manifest:
                manifest_data['files'].append(overridden_name)

        args.in_folder = get_chromium_src_override(args.in_folder)
        args.in_files = overrides
        return args

    try:
        # Try and run the function again - if we have no overrides this will
        # raise an exception so we don't reprocess the files.
        original_function(argv)

        # We need to update the manifest to include all the -chromium files.
        if manifest_path:
            with open(manifest_path, 'w', encoding='utf-8', newline='\n') as f:
                json.dump(manifest_data, f)

    except NoOverridesException:
        # we throw an exception when there are no overrides, so we don't reparse
        pass
