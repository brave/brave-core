# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

from signing.chromium_config import ChromiumCodeSignConfig
from signing.model import Distribution, NotarizeAndStapleLevel

BRAVE_CHANNEL = os.environ.get('BRAVE_CHANNEL')


class InternalCodeSignConfig(ChromiumCodeSignConfig):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.enable_universal_app_dir = False

    @staticmethod
    def is_chrome_branded():
        # We want to inherit most of upstream's behavior.
        return True

    @property
    def distributions(self):
        return [
            Distribution(channel=BRAVE_CHANNEL,
                         package_as_dmg=True,
                         package_as_pkg=True,
                         package_as_zip=True)
        ]

    @property
    def codesign_requirements_outer_app(self):
        return 'designated => identifier "' + self.base_bundle_id + '"'

    @property
    def codesign_requirements_basic(self):
        return 'and anchor apple generic and certificate 1[field.1.2.840.113635.100.6.2.6] /* exists / and certificate leaf[field.1.2.840.113635.100.6.1.13] / exists */'  # pylint: disable=line-too-long

    @property
    def provisioning_profile_basename(self):
        if not self.invoker.args.skip_signing:
            return BRAVE_CHANNEL or 'release'
        return None

    @property
    def run_spctl_assess(self):
        # It only makes sense to run spctl assess when the binary was notarized
        # and stapled. This implementation checks whether that is the case.
        #
        # It's tempting to use self._notarize here, but it does not contain the
        # correct value: When this object is a DistributionCodeSignConfig, then
        # its _notarize always has the default value STAPLE instead of the value
        # from the base config from which it was created. We therefore refer to
        # invoker.args.notarize, which does contain the correct value.
        return self.invoker.args.notarize == NotarizeAndStapleLevel.STAPLE

    @property
    def app_dir(self):
        app_dir_basename = super().app_dir
        if self.invoker.args.universal and self.enable_universal_app_dir:
            return 'universal/' + app_dir_basename
        return app_dir_basename
