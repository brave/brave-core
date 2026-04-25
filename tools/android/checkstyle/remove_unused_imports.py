# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os


def main():
    src_dir = os.path.abspath(
        os.path.join(__file__, os.pardir, os.pardir, os.pardir, os.pardir,
                     os.pardir))
    chromium_checkstyle_dir = os.path.join(src_dir, "tools", "android",
                                           "checkstyle")
    chromium_script = os.path.join(chromium_checkstyle_dir,
                                   "remove_unused_imports.py")
    chromium_style_path = os.path.join(chromium_checkstyle_dir,
                                       "unused-imports.xml")

    with open(chromium_script, "r") as f:
        contents = f.read()
        contents = contents.replace("main()", "chromium_main()")
        contents = contents.replace("if __name__ == '__main__':",
                                    "if __name__ != '__main__':")
        exec(contents, globals(), globals())  # pylint: disable=exec-used

    globals()['_STYLE_FILE'] = chromium_style_path
    globals()['chromium_main']()


if __name__ == '__main__':
    main()
