#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */


import requests

from lib.config import get_env_var
from lib.l10n.transifex.api_v2_wrapper import TransifexAPIV2Wrapper
from transifex.api import transifex_api  # pylint: disable=import-error


# This module is a wrapper around Transifex API v3


brave_organization_name = 'brave'


class TransifexAPIV3Wrapper(TransifexAPIV2Wrapper):
    """Wrapper class for the Transifex API (v3) python SDK from
       https://github.com/transifex/transifex-python"""
    # pylint: disable=super-init-not-called
    def __init__(self,
                 project_name,
                 organization_name = brave_organization_name):
        self._organization_name = organization_name
        self._project_name = project_name
        self._transifex_api_key = get_env_var('TRANSIFEX_API_KEY')
        assert self._organization_name, \
            'TransifexAPIV3Wrapper: organization_name is not set.'
        assert self._project_name, \
            'TransifexAPIV3Wrapper: project_name is not set.'
        assert self._transifex_api_key, \
            'TRANSIFEX_API_KEY environmental var is not set.'
        self.__auth()


    @property
    def organization_name(self):
        return self._organization_name


    @property
    def project_name(self):
        return self._project_name


    def __auth(self):
        """Set up auth to Transifex using an API token. You can generate one at
           https://www.transifex.com/user/settings/api/"""
        print('Authenticating with Transifex...')
        transifex_api.setup(auth=self._transifex_api_key)


    def __get_organization(self):
        return transifex_api.Organization.get(slug=self._organization_name)


    def __get_project(self):
        return self.__get_organization().fetch('projects').get(
            slug=self.project_name)


    def __get_project_languages(self):
        return self.__get_project().fetch('languages')


    # pylint: disable=no-self-use
    def __get_language(self, lang_code):
        return transifex_api.Language.get(code=lang_code)
    # pylint: enable=no-self-use


    def __is_supported_language(self, lang_code):
        language = self.__get_language(lang_code)
        project_languages = list(self.__get_project_languages())
        return language in project_languages


    def __get_resource(self, resource_slug):
        return self.__get_project().fetch('resources').get(slug=resource_slug)


    # Same as above, but when the resource doesn't exist the above will throw
    # a transifex.api.jsonapi.exceptions.DoesNotExist exception. Use this one
    # if it's unknown if the resource exists (such as when uploading resource
    # content).
    def __get_existing_resource(self, resource_slug):
        project = self.__get_project()
        resources = project.fetch('resources').filter(slug=resource_slug)
        if len(resources) > 0:
            assert len(resources) == 1, \
                f'Multiple resources named {resource_slug}'
            return resources[0]
        return None


    def __get_resource_strings_download_url(self, resource_slug):
        resource = self.__get_resource(resource_slug)
        return transifex_api.ResourceStringsAsyncDownload.download(
            resource=resource, content_encoding='text', file_type='default')


    def __get_resource_translation_download_url(self, resource_slug, lang_code):
        resource = self.__get_resource(resource_slug)
        language = self.__get_language(lang_code)
        return transifex_api.ResourceTranslationsAsyncDownload.download(
            resource=resource, language=language)


    def __get_i18_format(self, format_name):
        return transifex_api.I18nFormat.get(
            organization=self.__get_organization(), name=format_name)


    def __create_resource(self, resource_slug, i18n_type):
        i18n_format=self.__get_i18_format(i18n_type)
        return transifex_api.Resource.create(project=self.__get_project(),
            name=resource_slug, slug=resource_slug, i18n_format=i18n_format)


    # pylint: disable=no-self-use
    def __upload_resource_content(self, resource, content):
        transifex_api.ResourceStringsAsyncUpload.upload(resource, content)
    # pylint: enable=no-self-use


    def __get_resource_string_id(self, resource_slug, string_hash):
        return (f'o:{self.organization_name}:p:{self.project_name}:'
                f'r:{resource_slug}:s:{string_hash}')


    def __get_resource_string(self, resource_slug, string_hash):
        string_id = self.__get_resource_string_id(resource_slug, string_hash)
        return transifex_api.ResourceString.get(id = string_id)


    def __get_resource_translation_id(self, resource_slug, lang_code,
                                      string_hash):
        return (f'o:{self.organization_name}:p:{self.project_name}:'
                f'r:{resource_slug}:s:{string_hash}:l:{lang_code}')


    def __get_resource_translation(self, resource_slug, lang_code, string_hash):
        translation_id = self.__get_resource_translation_id(
            resource_slug, lang_code, string_hash)
        return transifex_api.ResourceTranslation.get(
            id = translation_id)


    # TransifexAPIWrapper overrides


    def transifex_get_resource_source(self, resource_name):
        """Downloads resource source strings (original language) from
           Transifex"""
        url = self.__get_resource_strings_download_url(resource_name)
        r = requests.get(url)
        assert r.status_code >= 200 and r.status_code <= 299, \
            f'Aborting. Status code {r.status_code}: {r.content}'
        r.encoding = 'utf-8'
        content = r.text.encode('utf-8')
        return content


    def transifex_get_resource_l10n(self, resource_name, lang_code, file_ext):
        """Downloads resource l10n from Transifex for the given language"""
        if not self.__is_supported_language(lang_code):
            print (f'WARNING: Language "{lang_code}" has not been added to'
                   f' the "{self.project_name}" project in Transifex')
            # For json files we need to have content even if untranslated, so
            # get the source strings instead
            if file_ext == '.json':
                url = self.__get_resource_strings_download_url(resource_name)
            elif file_ext == '.grd':
                # For GRDs we can just return an empty <resources> content:
                return '<resources></resources>'.encode('utf-8')
            else:
                assert False, f'Unexpected file extension {file_ext}'
        else:
            url = self.__get_resource_translation_download_url(
                resource_name, lang_code)
        r = requests.get(url)
        assert r.status_code >= 200 and r.status_code <= 299, \
            f'Aborting. Status code {r.status_code}: {r.content}'
        r.encoding = 'utf-8'
        content = r.text.encode('utf-8')
        return content


    def transifex_upload_resource_content(self, resource_name, xml_content,
                                          i18n_type):
        """Upload resource content to Transifex"""
        resource = self.__get_existing_resource(resource_name)
        if resource:
            print(f'Resource named {resource_name} already exists')
        else:
            print(f'Creating a new resource named {resource_name}')
            resource = self.__create_resource(resource_name, i18n_type)

        self.__upload_resource_content(resource, xml_content)
        return True


    def transifex_upload_string_desc(self, resource_name, string_hash,
                                     string_desc):
        """Uploads a description for the string with the given hash"""
        resource_string = self.__get_resource_string(resource_name, string_hash)
        resource_string.instructions = string_desc
        resource_string.save('instructions')


    def transifex_upload_string_l10n(self, resource_name, string_name,
                                     string_hash, lang_code, translated_value,
                                     missing_only):
        """Uploads the localized string in the given language for the string
           with the given hash."""
        try:
            translation = self.__get_resource_translation(
                resource_name, lang_code, string_hash)
        except: # pylint: disable=bare-except
            print(f'WARNING: String {string_name} (hash {string_hash}) is not '\
                  f'in resource {resource_name} for language {lang_code} in '\
                  'Transifex.')
            return
        if (missing_only and translation.strings is not None and
            len(translation.strings['other'])):
            #print(f'String {string_name} (hash {string_hash}) already has a ' \
            #      'translation for the language `{lang_code}` and ' \
            #      '--with_missing_translations flag was specified')
            return
        translation.strings = {'other': translated_value}
        translation.reviewed = True
        # `proofread` attribute cannot be set on project's not supporting
        # second review step - is the exception we'd get if try to set this.
        # Currently, https://www.transifex.com/brave/brave/settings/workflow
        # doesn't have `Proofread` option checked. Settings this in APIv2
        # doesn't appear to do anything when the option is turned off.
        # If we turn that option on make sure to add 'proofread' to the save
        # call below.
        # translation.proofread = True
        translation.save('strings', 'reviewed')
        print(f'Uploaded translation for {lang_code}: {string_name} ' \
              f'({string_hash})...')
