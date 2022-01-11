# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
"""Modifies DEPS checker to support `brave/chromium_src` overrides."""

import posixpath

from builddeps import DepsBuilder, NormalizePath


class BraveDepsBuilder(DepsBuilder):
  def _ApplyDirectoryRules(self, existing_rules, dir_path_local_abs):
    directory_rules, excluded_subdirs = super(DepsBuilder,
                                              self)._ApplyDirectoryRules(
                                                  existing_rules,
                                                  dir_path_local_abs)

    dir_path_norm = NormalizePath(dir_path_local_abs)
    if '/brave/chromium_src' in dir_path_norm:
      # Append rules from the original `src/...` dir.
      root_src_dir_path_norm = dir_path_norm.replace('/brave/chromium_src', '',
                                                     1)
      directory_rules, _ = super(DepsBuilder, self)._ApplyDirectoryRules(
          directory_rules, root_src_dir_path_norm)

      # Add `+src/...` rule.
      root_src_relative_dir = 'src/' + posixpath.relpath(
          root_src_dir_path_norm, NormalizePath(self.base_directory))
      directory_rules.AddRule('+' + root_src_relative_dir,
                              root_src_relative_dir,
                              'Src rule for ' + root_src_relative_dir)

      # Add `+../gen/...` rule.
      root_src_relative_dir = '../gen/' + posixpath.relpath(
          root_src_dir_path_norm, NormalizePath(self.base_directory))
      directory_rules.AddRule('+' + root_src_relative_dir,
                              root_src_relative_dir,
                              'Gen rule for ' + root_src_relative_dir)

    return directory_rules, excluded_subdirs


DepsBuilder = BraveDepsBuilder
