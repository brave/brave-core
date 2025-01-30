from signing import standard_invoker, signing


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
            stub_out_verify_part()
        # The config can use this to access the args:
        self.args = args


class StubSigner:
    def codesign(self, config, product, path):
        # Do nothing.
        pass


def stub_out_verify_part():
    # When we skip signing, then upstream's verify_part function fails at
    # various points of the pipeline. Upstream doesn't give us a way to
    # customize verify_part. This function therefore monkey-patches it.
    signing.verify_part = lambda *args, **kwargs: None
