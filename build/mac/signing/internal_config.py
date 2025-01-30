import os

from signing.chromium_config import ChromiumCodeSignConfig
from signing.model import Distribution

BRAVE_CHANNEL = os.environ.get('BRAVE_CHANNEL')

class InternalCodeSignConfig(ChromiumCodeSignConfig):

    @staticmethod
    def is_chrome_branded():
        return False

    @property
    def distributions(self):
        args = self.invoker.args
        return [Distribution(
            channel=BRAVE_CHANNEL,
            package_as_dmg=args.package_as_dmg,
            package_as_pkg=args.package_as_pkg
        )]

    @property
    def codesign_requirements_outer_app(self):
        return 'designated => identifier "' + self.base_bundle_id + '"'

    @property
    def codesign_requirements_basic(self):
        return 'and anchor apple generic and certificate 1[field.1.2.840.113635.100.6.2.6] /* exists / and certificate leaf[field.1.2.840.113635.100.6.1.13] / exists */' # pylint: disable=line-too-long

    @property
    def provisioning_profile_basename(self):
        return BRAVE_CHANNEL or 'release'
    
    @property
    def run_spctl_assess(self):
        return True
