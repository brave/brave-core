# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from signing import standard_invoker, signing, commands


class Invoker(standard_invoker.Invoker):

    @staticmethod
    def register_arguments(parser):
        standard_invoker.Invoker.register_arguments(parser)
        parser.add_argument("--package-as-dmg", action="store_true")
        parser.add_argument("--package-as-pkg", action="store_true")
        parser.add_argument("--skip-signing", action="store_true")

    def __init__(self, args, config):
        super().__init__(args, config)
        if args.skip_signing:
            self._signer = StubSigner()
            stub_out_signing_in_upstream()
        # The config can use this to access the args:
        self.args = args


class StubSigner:

    def codesign(self, config, product, path):
        # Do nothing.
        pass


def stub_out_signing_in_upstream():
    # When we skip signing, then upstream's verify_part function fails at
    # various points of the pipeline. Upstream doesn't give us a way to
    # customize verify_part. This function therefore monkey-patches it.
    signing.verify_part = lambda *args, **kwargs: None

    run_command_orig = commands.run_command

    def run_command(args, **kwargs):
        if args[0] == 'productbuild':
            try:
                sign_index = args.index('--sign')
            except ValueError:
                pass
            else:
                for _ in range(2):
                    args.pop(sign_index)
        return run_command_orig(args, **kwargs)

    commands.run_command = run_command
