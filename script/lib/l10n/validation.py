#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import lxml.etree  # pylint: disable=import-error


# This module contains functionality to validate XML content of strings


# List of HTML tags that we allow inside the translated text.
allowed_html_tags = [
    'a', 'abbr', 'b', 'b1', 'b2', 'br', 'code', 'h4', 'learnmore', 'li', 'li1',
    'li2', 'ol', 'p', 'span', 'strong', 'ul'
]

def validate_elements_tags(elements):
    """Recursively validates elements for being in the allow list"""
    errors = None
    for element in elements:
        if element.tag not in allowed_html_tags:
            error = f'ERROR: Element <{element.tag}> is not allowed.\n'
            errors = (errors or '') + error
        rec_errors = validate_elements_tags(list(element))
        if rec_errors is not None:
            errors = (errors or '') + rec_errors
    return errors


def validate_tags_in_one_string(string_tag, textify_callback):
    """Validates that all child elements of the |string_tag|'s content XML are
       allowed"""
    lxml.etree.strip_elements(string_tag, 'ph')
    string_text = textify_callback(string_tag)
    string_text = string_text.replace('&lt;', '<').replace('&gt;', '>')
    # print(f'Validating: {string_text.encode('utf-8')}')
    try:
        string_xml = lxml.etree.fromstring(
            '<string>' + string_text + '</string>')
    except lxml.etree.XMLSyntaxError as e:
        errors = '\n--------------------\n' \
            f"{string_text.encode('utf-8')}\nERROR: {str(e)}\n"
        print(errors)
        cont = input(
            'Enter C to ignore and continue. Enter anything else to exit : ')
        if cont in ('C', 'c'):
            return None
        return errors
    errors = validate_elements_tags(list(string_xml))
    if errors is not None:
        tag_text = lxml.etree.tostring(
            string_tag, method='xml', encoding='utf-8', pretty_print=True)
        errors = f'--------------------\n{tag_text}\n' + errors
    return errors
