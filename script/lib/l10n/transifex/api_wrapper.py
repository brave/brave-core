#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import requests

from lib.config import get_env_var


# This module is wrapper around Transifex API


# transifex_project_name = 'brave'
transifex_project_name = 'maxtest'
base_url = 'https://www.transifex.com/api/2/'


def check_env_vars():
    """ Checks that we can get authentication info from the environment"""
    transifex_info = (get_env_var('TRANSIFEX_USERNAME') if
                      get_env_var('TRANSIFEX_PASSWORD') else
                      get_env_var('TRANSIFEX_API_KEY'))
    message = 'TRANSIFEX_USERNAME and TRANSIFEX_PASSWORD or '\
              'TRANSIFEX_API_KEY must be set'
    assert transifex_info, message


def get_auth():
    """Creates an HTTPBasicAuth object given the Transifex information"""
    check_env_vars()
    username = get_env_var('TRANSIFEX_USERNAME')
    password = get_env_var('TRANSIFEX_PASSWORD')
    transifex_api_key = get_env_var('TRANSIFEX_API_KEY')
    auth = None
    if transifex_api_key:
        api_key_username = "api:" + transifex_api_key
        auth = requests.auth.HTTPBasicAuth(api_key_username, '')
    else:
        auth = requests.auth.HTTPBasicAuth(username, password)
    return auth


def transifex_get_resource_source(resource_name):
    """Downloads resource source strings (original language) from Transifex"""
    url_part = (f'project/{transifex_project_name}/resource/'
                f'{resource_name}/content/')
    url = base_url + url_part
    r = requests.get(url, auth=get_auth())
    assert r.status_code >= 200 and r.status_code <= 299, \
        f'Aborting. Status code {r.status_code}: {r.content}'
    content = r.json()['content'].encode('utf-8')
    return content


def transifex_get_resource_l10n(resource_name, lang_code):
    """Downloads resource l10n from Transifex for the given language"""
    url_part = f'project/{transifex_project_name}/resource/{resource_name}' \
               f'/translation/{lang_code}?mode=default'
    url = base_url + url_part
    r = requests.get(url, auth=get_auth())
    assert r.status_code >= 200 and r.status_code <= 299, \
        f'Aborting. Status code {r.status_code}: {r.content}'
    content = r.json()['content'].encode('utf-8')
    return content


def transifex_upload_resource_content(resource_name, xml_content, i18n_type):
    """Upload new resource content to Transifex"""
    print(f'Creating a new resource named {resource_name}')
    url_part = f'project/{transifex_project_name}/resources/'
    url = base_url + url_part
    payload = {
        'name': resource_name,
        'slug': resource_name,
        'content': xml_content,
        'i18n_type': i18n_type
    }
    headers = {'Content-Type': 'application/json'}
    r = requests.post(url, json=payload, auth=get_auth(), headers=headers)
    if r.status_code < 200 or r.status_code > 299:
        if r.content.find(
                b'Resource with this Slug and Project already exists.') != -1:
            print(f'Resource named {resource_name} already exists')
            return update_source_string_file_to_transifex(resource_name,
                                                          xml_content)
        assert False, f'Aborting. Status code {r.status_code}: {r.content}'
    return True


def update_source_string_file_to_transifex(resource_name, xml_content):
    """Uploads the specified source string file to transifex"""
    print(f'Updating existing resource named {resource_name}')
    url_part = (f'project/{transifex_project_name}/resource/'
                f'{resource_name}/content')
    url = base_url + url_part
    payload = {'content': xml_content}
    headers = {'Content-Type': 'application/json'}
    r = requests.put(url, json=payload, auth=get_auth(), headers=headers)
    assert r.status_code >= 200 and r.status_code <= 299, \
        f'Aborting. Status code {r.status_code}: {r.content}'
    return True


def transifex_upload_string_desc(resource_name, string_hash, string_desc):
    """Uploads a description for the string with the given hash"""
    url_part = (f'project/{transifex_project_name}/resource/'
                f'{resource_name}/source/{string_hash}')
    url = base_url + url_part
    payload = {
        'comment': string_desc,
    }
    headers = {'Content-Type': 'application/json'}
    r = requests.put(url, json=payload, auth=get_auth(), headers=headers)
    if r.status_code == 400 and b'Source string does not exist' in r.content:
        print('WARNING: Source string with hash {string_hash} does not exist')
        return
    assert r.status_code >= 200 and r.status_code <= 299, \
        f'Aborting. Status code {r.status_code}: {r.content}'


def transifex_upload_string_l10n(resource_name, string_hash, lang_code,
                                 translated_value):
    """Uploads the localized string in the given language for the string with
       the given hash."""
    url_part = (f'project/{transifex_project_name}/resource/{resource_name}'
                f'/translation/{lang_code}/string/{string_hash}/')
    url = base_url + url_part
    payload = {
        'translation': translated_value,
        # Assume Chromium provided strings are reviewed and proofread
        'reviewed': True,
        'proofread': True,
        'user': 'bbondy'
    }
    headers = {'Content-Type': 'application/json'}
    r = requests.put(url, json=payload, auth=get_auth(), headers=headers)
    assert r.status_code >= 200 and r.status_code <= 299, \
        f'Aborting. Status code {r.status_code}: {r.content}'
