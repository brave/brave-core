# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from os import path
import re
import requests

font_regex = re.compile(
    r"\/\* (.*) \*\/\n@font-face {\n\s+font-family: '(.*)';(?:\s|.)+?\
      font-weight: (\d+);(?:\s|.)+?src: url\((.*?)\)(?:.|\s)+?}", re.MULTILINE)
headers = {
    'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, \
                   like Gecko) Chrome/120.0.0.0 Safari/537.36',
}


def download_font_file(url: str, name: str):
    with open(name, 'wb') as f:
        response = requests.get(url, headers=headers)
        if not response.ok:
            raise Exception(f"Oh no! (response was {response.status_code})")
        f.write(response.content)


def download_font(font_name: str, css_url: str):
    css = requests.get(css_url, headers=headers).text

    for (charset, name, weight, url) in re.findall(font_regex, css):
        name = name.lower()

        font_file_name = f'{name}-{charset}-{weight}.woff2'
        font_file_path = path.join('third_party', name, font_file_name)
        download_font_file(url, font_file_path)

        css = css.replace(url, f'./third_party/{name}/{font_file_name}')

    with open(f'{font_name}.css', 'w') as f:
        f.write(css)


# This downloads all the font files necessary to make a Google font work and
# puts them in the ./third_party/{font}/{font}-{charset}-{weight}.woff2.
# Some fonts (such as Inter) require an additional manual step to rename the
# font-family (on Inter we use 'Inter Variable' as the family name, instead of
# plain old 'Inter').
download_font(
    'inter', "https://fonts.googleapis.com/css2?family=Inter:wght@\
     100;200;300;400;500;600;700;800;900&display=swap")
