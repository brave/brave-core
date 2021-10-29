import json
import os
import re
import requests

from argparse import RawTextHelpFormatter
from distutils.version import LooseVersion
from functools import cmp_to_key


def call_github_api(url, headers):
    try:
        r = requests.get(url, headers=headers)
    except requests.exceptions.ConnectionError as e:
        print("Error: Received requests.exceptions.ConnectionError, Exiting...")
        exit(1)
    except Exception as e:
        raise Exception(e)

    if r.status_code is 200:
        return r


def loose_version_cmp(a, b):
    if LooseVersion(a) == LooseVersion(b):
        return 0
    if LooseVersion(a) > LooseVersion(b):
        return 1
    if LooseVersion(a) < LooseVersion(b):
        return -1


def get_github_tags(branch):

    GITHUB_URL = 'https://api.github.com'

    tags = []

    next_request = ""
    headers = {'Accept': 'application/vnd.github+json',
               'Authorization': 'token ' + os.environ.get('BRAVE_GITHUB_TOKEN')
               }
    tag_url = GITHUB_URL + "/repos/brave/brave-core/tags" + '?page=1&per_page=100'

    r = call_github_api(tag_url, headers=headers)
    next_request = ""

    # The GitHub API returns paginated results of 100 items maximum per
    # response. We will loop until there is no next link header returned
    # in the response header. This is documented here:
    # https://developer.github.com/v3/#pagination
    while next_request is not None:
        for item in r.json():
            match = re.search(branch, item['name'])
            if match:
                tags.append(item['name'])
        if r.links.get("next"):
            next_request = r.links["next"]["url"]
            r = call_github_api(next_request, headers=headers)
        else:
            next_request = None

    return tags


def main():
    branch = os.environ.get('CHANNEL_BRANCH')
    match = re.search(r'\d+[.]\d+', branch)
    if match:
        branch = match.group(0)
    else:
        print("Error: Malformed branch \'{}\'".format(branch))
        exit(1)

    items = get_github_tags(branch)

    sorted_list = sorted(items, key=cmp_to_key(
        loose_version_cmp), reverse=True)
    print(sorted_list[0])


if __name__ == '__main__':
    import sys
    sys.exit(main())
