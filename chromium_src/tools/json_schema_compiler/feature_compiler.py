# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

#pylint: disable=line-too-long,too-few-public-methods,undefined-variable,protected-access

import import_inline
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
    }

    @classmethod
    def GetFeatureDefinitionsToAdd(cls, source_file):
        has_counterpart = cls._HasBraveChromiumSrcConterpart(source_file)
        overridden_filepath = import_inline.wspath(
            f"//brave/chromium_src/{source_file}")
        assert has_counterpart == os.path.exists(
            overridden_filepath), overridden_filepath
        if not has_counterpart:
            return None

        with open(overridden_filepath, "r") as f:
            parsed_json = json_parse.Parse(f.read())
        for feature, definitions in parsed_json.items():
            assert isinstance(definitions, list), (overridden_filepath, feature)
        return parsed_json

    @classmethod
    def _HasBraveChromiumSrcConterpart(cls, source_file):
        has_counterpart = cls.KNOWN_FILES.get(source_file)
        if has_counterpart is None:
            raise RuntimeError(
                f"Unknown features file {source_file}. Please update "
                "//brave/chromium_src/tools/json_schema_compiler/feature_compiler.py"
            )
        return has_counterpart


@override_utils.override_method(FeatureCompiler)
def Load(self, original_method):
    original_method(self)

    parent_dir_prefix = "../../"

    for source_file in self._source_files:
        assert source_file.startswith(parent_dir_prefix), source_file
        source_file = source_file[len(parent_dir_prefix):]

        # Skip files that we never override.
        if source_file.startswith("brave/") or "/test/" in source_file:
            continue

        feature_definitions_to_add = BraveFeatureDefinitionExtender. \
            GetFeatureDefinitionsToAdd(source_file)
        if feature_definitions_to_add:
            for feature, definitions in feature_definitions_to_add.items():
                existing_definitions = self._json.get(feature, None)
                if existing_definitions is None:
                    raise RuntimeError(
                        f"Feature {feature} not found in {source_file}")

                # Convert to ComplexFeature if it's currently a SimpleFeature.
                if not isinstance(existing_definitions, list):
                    existing_definitions = [existing_definitions]

                # Add new definitions.
                existing_definitions.extend(definitions)

                self._json[feature] = existing_definitions
