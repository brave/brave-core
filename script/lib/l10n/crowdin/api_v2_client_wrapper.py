#!/usr/bin/env python3
#
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import requests

from lib.config import get_env_var
# pylint: disable=import-error
from crowdin_api import CrowdinClient
from crowdin_api.api_resources.source_files.enums import FileType
# pylint: enable=import-error

# This module is a wrapper around Crowdin API v2


class CrowdinClientWrapper():
    """Wrapper class for the Crowdin API (v2) python SDK from
       https://github.com/crowdin/crowdin-api-client-python"""

    def __init__(self, project_id):
        self._organization = 'Brave-Software'
        self._project_id = project_id
        self._auth_token = get_env_var('CROWDIN_API_KEY')
        assert self._project_id, \
            'CrowdinClientWrapper: project_id is not set.'
        assert self._auth_token, \
            'BRAVE_CROWDIN_API_KEY environmental var is not set.'
        # Set up CrowdinClient using an API token. You can generate one at
        # https://brave-software.crowdin.com/u/user_settings/access-tokens
        self._client = CrowdinClient(organization=self._organization,
                                     project_id=self.project_id,
                                     token=self._auth_token)

    @property
    def project_id(self):
        return self._project_id

    def __get_branch(self, branch_name):
        all_branches = self._client.source_files.list_project_branches(
            projectId=self._project_id)['data']
        for branch_data in all_branches:
            branch = branch_data['data']
            if branch['name'] == branch_name:
                return branch['id']
        return 0

    def __create_branch(self, branch_name):
        branch = self._client.source_files.add_branch(
            name=branch_name, projectId=self._project_id)
        return branch['data']['id']

    def __create_storage(self, resource_path):
        storage_data = self._client.storages.add_storage(
            open(resource_path, 'rb'))
        return storage_data['data']['id']

    def __get_resource_file(self, branch_id, resource_name):
        all_files = self._client.source_files.list_files(
            projectId=self._project_id, branchId=branch_id)['data']
        for file_data in all_files:
            file = file_data['data']
            if file['name'] == resource_name:
                return file['id']
        return 0

    def __add_resource_file(self, branch_id, storage_id, resource_name,
                            file_type):
        file_types_map = {
            'ANDROID': FileType.ANDROID,
            'CHROME': FileType.CHROME
        }
        assert file_type in file_types_map, ('Unexpected file type: ' +
                                             f'{file_type}.')

        new_file = self._client.source_files.add_file(
            storageId=storage_id,
            name=resource_name,
            projectId=self._project_id,
            branchId=branch_id,
            type=file_types_map[file_type])
        return new_file['data']['id']

    def __update_resource_file(self, file_id, storage_id):
        updated_file = self._client.source_files.update_file(
            file_id, storageId=storage_id, projectId=self._project_id)
        return updated_file['data']['id']

    def __get_resource_download_url(self, file_id):
        download = self._client.source_files.download_file(
            fileId=file_id, projectId=self._project_id)
        return download['data']['url']

    def __get_resource_translation_download_url(self, file_id, lang_code):
        download = self._client.translations.export_project_translation(
            targetLanguageId=lang_code,
            projectId=self._project_id,
            fileIds=[file_id],
            skipUntranslatedStrings=True)
        return download['data']['url']

    def __get_resource_file_strings(self, file_id):
        return \
            self._client.source_strings.with_fetch_all().list_strings(
                projectId=self._project_id, fileId=file_id)['data']

    def __get_string_id_from_key(self, all_strings, string_key):
        for string_data in all_strings:
            string = string_data['data']
            if string['identifier'] == string_key:
                return string['id']
        return 0

    def __has_source_string_l10n(self, string_id, lang_code):
        all_translations = \
            self._client.string_translations.list_string_translations(
                projectId=self._project_id, stringId=string_id,
                languageId=lang_code)['data']
        return len(all_translations) and \
            len(all_translations[0]['data']['text'])

    def __delete_source_string_l10n(self, string_id, lang_code):
        self._client.string_translations.delete_string_translations(
            projectId=self._project_id,
            stringId=string_id,
            languageId=lang_code)

    def __add_source_string_l10n(self, string_id, lang_code, translation):
        self._client.string_translations.add_translation(
            projectId=self._project_id,
            stringId=string_id,
            languageId=lang_code,
            text=translation)

    def __upload_translation(self, file_id, storage_id, lang_code):
        uploaded_file = self._client.translations.upload_translation(
            projectId=self._project_id,
            languageId=lang_code,
            storageId=storage_id,
            fileId=file_id,
            importEqSuggestions=True,  # Add l10n == source
            autoApproveImported=True,
            translateHidden=True)
        return uploaded_file['data']['fileId']

    # Wrapper API

    def is_supported_language(self, lang_code):
        project = self._client.projects.get_project(
            projectId=self._project_id)['data']
        return lang_code in project['targetLanguageIds']

    def upload_resource_file(self, branch, upload_file_path, resource_name,
                             i18n_type):
        """Upload resource file to Crowdin"""
        # Create new storage for the file
        storage_id = self.__create_storage(upload_file_path)
        # Check if the branch already exists
        branch_id = self.__get_branch(branch)
        if branch_id:
            print(f'Branch {branch} already exists')
            # Check if this file already exists and if so update it
            file_id = self.__get_resource_file(branch_id, resource_name)
            if file_id:
                print(f'Resource {resource_name} already exists. Updating...')
                return self.__update_resource_file(file_id, storage_id)
        else:
            # Create new branch
            print(f'Creating new branch {branch}')
            branch_id = self.__create_branch(branch)

        print(f'Creating a new resource {resource_name}')
        file_id = self.__add_resource_file(branch_id, storage_id,
                                           resource_name, i18n_type)
        return file_id

    def get_resource_source(self, branch, resource_name):
        """Downloads resource source file (original language) from
           Crowdin"""
        branch_id = self.__get_branch(branch)
        assert branch_id, (
            f'Unable to get resource {resource_name} for ' +
            f'branch {branch} because the branch doesn\'t exist')
        file_id = self.__get_resource_file(branch_id, resource_name)
        assert file_id, (
            f'Unable to get resource {resource_name} for ' +
            f'branch {branch} because the resource doesn\'t exist')
        url = self.__get_resource_download_url(file_id)
        r = requests.get(url, timeout=10)
        assert r.status_code == 200, \
            f'Aborting. Status code {r.status_code}: {r.content}'
        r.encoding = 'utf-8'
        content = r.text.encode('utf-8')
        return content

    def get_resource_l10n(self, branch, resource_name, lang_code, file_ext):
        """Downloads resource l10n from Crowdin for the given language"""
        assert file_ext in ('.grd',
                            '.json'), (f'Unexpected file extension {file_ext}')
        if self.is_supported_language(lang_code):
            branch_id = self.__get_branch(branch)
            assert branch_id, (
                f'Unable to get {resource_name} l10n for ' +
                f'branch {branch} because the branch doesn\'t exist')
            file_id = self.__get_resource_file(branch_id, resource_name)
            assert file_id, (
                f'Unable to get {resource_name} l10n for ' +
                f'branch {branch} because the resource doesn\'t exist')
            url = self.__get_resource_translation_download_url(
                file_id, lang_code)
            r = requests.get(url, timeout=10)
            assert r.status_code == 200 or r.status_code == 204, \
                f'Aborting. Status code {r.status_code}: {r.content}'
            if r.status_code == 200:
                r.encoding = 'utf-8'
                if file_ext == '.grd':
                    # Remove xml declaration header
                    second_line = r.text.find('\n') + 1
                    text = r.text[second_line:]
                else:
                    text = r.text
                content = text.encode('utf-8')
                return content
        # Either unsupported language or status_code == 204 which means the
        # file is empty.
        if file_ext == '.json':
            # For json files we need to have content even if untranslated, so
            # get the source strings instead.
            return self.get_resource_source(branch, resource_name)
        # For GRDs we can just return an empty <resources> content:
        return '<resources></resources>'.encode('utf-8')

    def upload_strings_l10n(self, branch, resource_name, translations,
                            missing_only):
        """Upload translations"""
        branch_id = self.__get_branch(branch)
        assert branch_id, (
            f'Unable to get resource {resource_name} for ' +
            f'branch {branch} because the branch doesn\'t exist')
        file_id = self.__get_resource_file(branch_id, resource_name)
        assert file_id, (
            f'Unable to get resource {resource_name} for ' +
            f'branch {branch} because the resource doesn\'t exist')
        all_strings = self.__get_resource_file_strings(file_id)
        # Translation is a dictionary whose keys are the string keys and values
        # are lists of tuples of language codes and translation strings.
        total = len(translations.keys())
        for idx, string_key in enumerate(translations.keys()):
            string_id = self.__get_string_id_from_key(all_strings, string_key)
            assert string_id, (f'Unable to find string by key {string_key} ' +
                               f'in resource {resource_name}.')
            print(f'[{idx + 1}/{total}] Uploading translations for key ' +
                  f'{string_key}')

            for lang_code, translation in translations[string_key]:
                has_l10n = self.__has_source_string_l10n(string_id, lang_code)
                if has_l10n:
                    if missing_only:
                        print(f'  Skipping {lang_code}: already translated.')
                        continue
                    self.__delete_source_string_l10n(string_id, lang_code)
                print(f'  Uploading {lang_code}')
                self.__add_source_string_l10n(string_id, lang_code,
                                              translation)

    def upload_grd_l10n_file(self, branch, upload_file_path, resource_name,
                             lang):
        """Upload grd l10n file to Crowdin"""
        # Create new storage for the file
        storage_id = self.__create_storage(upload_file_path)
        # Check if the branch already exists
        branch_id = self.__get_branch(branch)
        assert branch_id, f'Branch {branch} doesn\'t exist.'
        file_id = self.__get_resource_file(branch_id, resource_name)
        assert file_id, f'Resource {resource_name} doesn\'t exists.'
        return self.__upload_translation(file_id, storage_id, lang)
