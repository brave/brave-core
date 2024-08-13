# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import brave_chromium_utils
import override_utils


class BraveFeatureDefinitionExtender:
    # This is executed for all important files. List all of them so we can fail
    # in case something new appears in the repository or if an existing file is
    # being moved somewhere. "True" means //brave/chromium_src/... counterpart
    # must exist.
    KNOWN_FILES = {
        "chrome/common/apps/platform_apps/api/_api_features.json": False,
        "chrome/common/apps/platform_apps/api/_permission_features.json": False,
        "chrome/common/controlled_frame/api/_api_features.json": False,
        "chrome/common/extensions/api/_api_features.json": True,
        "chrome/common/extensions/api/_manifest_features.json": True,
        "chrome/common/extensions/api/_permission_features.json": True,
        "extensions/common/api/_api_features.json": True,
        "extensions/common/api/_behavior_features.json": True,
        "extensions/common/api/_manifest_features.json": True,
        "extensions/common/api/_permission_features.json": True,
        "extensions/shell/common/api/_api_features.json": False,
    }

    def __init__(self):
        self._ValidateKnownFiles()

    def GetFeatureDefinitions(self, source_file):
        has_counterpart = self._HasBraveChromiumSrcConterpart(source_file)
        if not has_counterpart:
            return None

        with open(
                brave_chromium_utils.wspath(
                    f"//brave/chromium_src/{source_file}"), "r") as f:
            parsed_json = json_parse.Parse(f.read())
        return parsed_json

    def _HasBraveChromiumSrcConterpart(self, source_file):
        has_counterpart = self.KNOWN_FILES.get(source_file)
        if has_counterpart is None:
            raise RuntimeError(
                f"Unknown features file {source_file}. Please update "
                f"{brave_chromium_utils.get_chromium_src_override(__file__)}")
        return has_counterpart

    def _ValidateKnownFiles(self):
        for source_file, should_exist in self.KNOWN_FILES.items():
            # Ensure original file exists.
            original_filepath = brave_chromium_utils.wspath(f"//{source_file}")
            if not os.path.exists(original_filepath):
                raise RuntimeError(
                    f"Original features file {original_filepath} not found. Please update "
                    f"{brave_chromium_utils.get_chromium_src_override(__file__)}"
                )
            # Ensure override file exists if it has to.
            overridden_filepath = brave_chromium_utils.wspath(
                f"//brave/chromium_src/{source_file}")
            assert should_exist == os.path.exists(
                overridden_filepath), overridden_filepath


@override_utils.override_method(FeatureCompiler)
def Load(self, original_method):
    original_method(self)

    brave_extender = BraveFeatureDefinitionExtender()
    parent_dir_prefix = "../../"
    feature_replace_prefix = "replace!"

    for source_file in self._source_files:
        assert source_file.startswith(parent_dir_prefix), source_file
        source_file = source_file[len(parent_dir_prefix):]

        # Skip files that we never override.
        if source_file.startswith("brave/") or "/test/" in source_file:
            continue

        feature_definitions = brave_extender.GetFeatureDefinitions(source_file)
        if feature_definitions:
            for feature, definitions in feature_definitions.items():
                should_replace = feature.startswith(feature_replace_prefix)
                if should_replace:
                    feature = feature[len(feature_replace_prefix):]

                existing_definitions = self._json.get(feature, None)
                if existing_definitions is None:
                    raise RuntimeError(
                        f"Feature {feature} not found in {source_file}")

                if should_replace:
                    # Fully replace definitions.
                    self._json[feature] = definitions
                else:
                    assert isinstance(definitions, list), feature
                    # Convert to ComplexFeature if it's currently a
                    # SimpleFeature.
                    if not isinstance(existing_definitions, list):
                        existing_definitions = [existing_definitions]

                    # Add new definitions.
                    existing_definitions.extend(definitions)

                    self._json[feature] = existing_definitions
