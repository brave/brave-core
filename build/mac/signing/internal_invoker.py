# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Upstream's signing and PKG/DMG/ZIP generation logic lets embedders hook into
# the process by providing a module named `signing.internal_invoker` with a
# class named `Invoker`. This file provides such code to apply customizations
# that are necessary for Brave. It collaborates with the similar hook
# `internal_config.py` in this directory.

from signing import standard_invoker, commands


class Invoker(standard_invoker.Invoker):

    @staticmethod
    def register_arguments(parser):
        standard_invoker.Invoker.register_arguments(parser)
        parser.add_argument("--skip_signing", action="store_true")
        parser.add_argument("--universal", action="store_true")
        parser.add_argument("--provisioning_profile_basename")

    def __init__(self, args, config):
        super().__init__(args, config)
        if args.skip_signing:
            stub_out_signing_in_upstream()
        # The config can use this to access the args:
        self.args = args


def stub_out_signing_in_upstream():
    run_command_orig = commands.run_command

    def run_command(args, **kwargs):
        if args[0] == 'codesign':
            # Even non-signing commands such as `codesign --verify` or
            # `codesign --display` fail when signing is skipped. So don't invoke
            # codesign at all:
            return
        if args[0] == 'productbuild':
            try:
                sign_index = args.index('--sign')
            except ValueError:
                pass
            else:
                for _ in range(2):
                    args.pop(sign_index)
        run_command_orig(args, **kwargs)

    commands.run_command = run_command
