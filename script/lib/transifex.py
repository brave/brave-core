from hashlib import md5
from collections import defaultdict
import HTMLParser
import io
import json
import os
import re
import tempfile
import requests
from lib.config import get_env_var
from lib.grd_string_replacements import (generate_braveified_node,
                                         get_override_file_path)
import lxml.etree # pylint: disable=import-error
import FP

# pylint: disable=too-many-locals

transifex_project_name = 'brave'
base_url = 'https://www.transifex.com/api/2/'
transifex_handled_slugs = [
    'android_brave_strings',
    'brave_generated_resources',
    'brave_components_resources',
    'brave_extension',
    'rewards_extension',
    'ethereum_remote_client_extension'
]
# List of HTML tags that we allowed to be present inside the translated text.
allowed_html_tags = [
    'a', 'abbr', 'b', 'b1', 'b2', 'br', 'code', 'h4', 'learnmore', 'li', 'li1',
    'li2', 'ol', 'p', 'span', 'strong', 'ul'
]


def transifex_name_from_greaselion_script_name(script_name):
    match = re.search(
        'brave-site-specific-scripts/scripts/(.*)/_locales/en_US/messages.json$', script_name)
    if match:
        return 'greaselion_' + match.group(1).replace('-', '_').replace('/', '_')
    return ''


def transifex_name_from_filename(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    if 'brave_components_strings' in source_file_path:
        return 'brave_components_resources'
    elif ext == '.grd':
        return filename
    elif 'brave-site-specific-scripts/scripts/' in source_file_path:
        return transifex_name_from_greaselion_script_name(source_file_path)
    elif 'brave_extension' in source_file_path:
        return 'brave_extension'
    elif 'brave_rewards' in source_file_path:
        return 'rewards_extension'
    elif 'ethereum-remote-client/app' in source_file_path:
        return 'ethereum_remote_client_extension'
    assert False, ('JSON files should be mapped explicitly, this '
                   'one is not: ' + source_file_path)


def create_xtb_format_translationbundle_tag(lang):
    """Creates the root XTB XML element"""
    translationbundle_tag = lxml.etree.Element('translationbundle')
    # The lang code "iw" is the old code for Hebrew, the GRDs are updated to
    # use "he".
    # But Chromium still uses "iw" inside the XTB, and it causes a compiling
    # error on Windows otherwise. So we need to force it back to "iw" here
    # for minimal impact.
    translationbundle_tag.set(
        'lang', lang.replace('_', '-').replace('he', 'iw').replace('sr-BA@latin', 'sr-Latn'))
    # Adds a newline so the first translation isn't glued to the
    # translationbundle element for us weak humans.
    translationbundle_tag.text = '\n'
    return translationbundle_tag


def create_xtb_format_translation_tag(fingerprint, string_value):
    """Creates child XTB elements for each translation tag"""
    string_tag = lxml.etree.Element('translation')
    string_tag.set('id', str(fingerprint))
    if string_value.count('<') != string_value.count('>'):
        assert False, 'Warning: Unmatched < character, consider fixing on ' \
                      ' Trasifex, force encoding the following string:' + string_value
    string_tag.text = string_value
    string_tag.tail = '\n'
    return string_tag


def create_android_format_resources_tag():
    """Creates intermediate Android format root tag"""
    return lxml.etree.Element('resources')


def create_android_format_string_tag(string_name, string_value):
    """Creates intermediate Android format child tag for
    each translation string"""
    string_tag = lxml.etree.Element('string')
    string_tag.set('name', string_name)
    string_tag.text = string_value
    string_tag.tail = '\n'
    return string_tag


def get_auth():
    """Creates an HTTPBasicAuth object given the Transifex information"""
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


def get_transifex_languages(grd_file_path):
    """Extracts the list of locales supported by the passed in GRD file"""
    xtb_files = get_xtb_files(grd_file_path)
    return set([lang for (lang, _) in xtb_files])


def get_transifex_translation_file_content(source_file_path, filename,
                                           lang_code):
    """Obtains a translation Android xml format and returns the string"""
    lang_code = lang_code.replace('-', '_')
    lang_code = lang_code.replace('iw', 'he')
    lang_code = lang_code.replace('sr_Latn', 'sr_BA@latin')
    url_part = 'project/%s/resource/%s/translation/%s?mode=default' % (
        transifex_project_name,
        transifex_name_from_filename(source_file_path, filename), lang_code)
    url = base_url + url_part
    r = requests.get(url, auth=get_auth())
    assert r.status_code >= 200 and r.status_code <= 299, (
        'Aborting. Status code %d: %s' % (r.status_code, r.content))
    content = r.json()['content'].encode('utf-8')
    ext = os.path.splitext(source_file_path)[1]
    if ext == '.json':
        # For .json files, for some reason Transifex puts a \'
        content = content.replace("\\'", "'")
        # Make sure it's parseable
        json.loads(content)
    elif ext == '.grd':
        # For .grd and .json files, for some reason Transifex puts a \\" and \'
        content = content.replace('\\\\"', '"').replace('\\"', '"').replace("\\'", "'")
        # Make sure it's parseable
        lxml.etree.fromstring(content)
    return content


def process_bad_ph_tags_for_one_string(val):
    val = (val.replace('\r\n', '\n')
           .replace('\r', '\n'))
    if val.find('&lt;ph') == -1:
        return val
    val = (val.replace('&lt;', '<')
           .replace('&gt;', '>')
           .replace('>  ', '> ')
           .replace('  <', ' <'))
    return val


def fixup_bad_ph_tags_from_raw_transifex_string(xml_content):
    begin_index = 0
    while begin_index < len(xml_content) and begin_index != -1:
        string_index = xml_content.find('<string', begin_index)
        if string_index == -1:
            return xml_content
        string_index = xml_content.find('>', string_index)
        if string_index == -1:
            return xml_content
        string_index += 1
        string_end_index = xml_content.find('</string>', string_index)
        if string_end_index == -1:
            return xml_content
        before_part = xml_content[:string_index]
        ending_part = xml_content[string_end_index:]
        val = process_bad_ph_tags_for_one_string(xml_content[string_index:string_end_index])
        xml_content = before_part + val + ending_part
        begin_index = xml_content.find('</string>', begin_index)
        if begin_index != -1:
            begin_index += 9
    return xml_content


def validate_elements_tags(elements):
    """Recursively validates elements for being in the allow list"""
    errors = None
    for element in elements:
        if element.tag not in allowed_html_tags:
            error = ("ERROR: Element <{0}> is not allowed.\n").format(element.tag)
            errors = (errors or '') + error
        rec_errors = validate_elements_tags(list(element))
        if rec_errors is not None:
            errors = (errors or '') + rec_errors
    return errors


def validate_tags_in_one_string(string_tag):
    """Validates that all child elements of a <string> are allowed"""
    lxml.etree.strip_elements(string_tag, 'ex', with_tail=False)
    lxml.etree.strip_tags(string_tag, 'ph')
    string_text = textify_from_transifex(string_tag)
    string_text = (string_text.replace('&lt;', '<')
                   .replace('&gt;', '>'))
    # print 'Validating: {}'.format(string_text.encode('utf-8'))
    try:
        string_xml = lxml.etree.fromstring('<string>' + string_text + '</string>')
    except lxml.etree.XMLSyntaxError as e:
        errors = ("\n--------------------\n"
                  "{0}\nERROR: {1}\n").format(string_text.encode('utf-8'), str(e))
        print errors
        cont = raw_input('Enter C to ignore and continue. Enter anything else to exit : ')
        if cont == 'C' or cont == 'c':
            return None
        return errors
    errors = validate_elements_tags(list(string_xml))
    if errors is not None:
        errors = ("--------------------\n"
                  "{0}\n").format(
                      lxml.etree.tostring(
                          string_tag, method='xml', encoding='utf-8', pretty_print=True)
                  ) + errors
    return errors


def validate_tags_in_transifex_strings(xml_content):
    """Validates that all child elements of all <string>s are allowed"""
    xml = lxml.etree.fromstring(xml_content)
    string_tags = xml.findall('.//string')
    # print 'Validating HTML tags in {} strings'.format(len(string_tags))
    errors = None
    for string_tag in string_tags:
        error = validate_tags_in_one_string(string_tag)
        if error is not None:
            errors = (errors or '') + error
    if errors is not None:
        errors = ("\n") + errors
    return errors

def trim_ph_tags_in_xtb_file_content(xml_content):
    """Removes all children of <ph> tags including text inside ph tag"""
    xml = lxml.etree.fromstring(xml_content)
    phs = xml.findall('.//ph')
    for ph in phs:
        lxml.etree.strip_elements(ph, '*')
        if ph.text is not None:
            ph.text = ''
    return lxml.etree.tostring(xml)


def get_strings_dict_from_xml_content(xml_content):
    """Obtains a dictionary mapping the string name to text from Android xml
    content"""
    strings = lxml.etree.fromstring(xml_content).findall('string')
    return {string_tag.get('name'): textify_from_transifex(string_tag)
            for string_tag in strings}


def get_strings_dict_from_xtb_file(xtb_file_path):
    """Obtains a dictionary mapping the string fingerprint to its value for
    an xtb file"""
    # No file exists yet, so just returna an empty dict
    if not os.path.isfile(xtb_file_path):
        return {}
    translation_tags = lxml.etree.parse(
        xtb_file_path).findall('.//translation')
    return {translation_tag.get('id'): textify(translation_tag)
            for translation_tag in translation_tags}


def update_source_string_file_to_transifex(source_file_path, filename,
                                           content):
    """Uploads the specified source string file to transifex"""
    print 'Updating existing known resource for filename %s' % filename
    url_part = 'project/%s/resource/%s/content' % (
        transifex_project_name, transifex_name_from_filename(
            source_file_path, filename))
    url = base_url + url_part
    payload = {'content': content}
    headers = {'Content-Type': 'application/json'}
    r = requests.put(url, json=payload, auth=get_auth(), headers=headers)
    assert r.status_code >= 200 and r.status_code <= 299, (
        'Aborting. Status code %d: %s' % (r.status_code, r.content))
    return True


def upload_source_string_file_to_transifex(source_file_path, filename,
                                           xml_content, i18n_type):
    """Uploads the specified source string file to transifex"""
    url_part = 'project/%s/resources/' % transifex_project_name
    url = base_url + url_part
    payload = {
        'name': transifex_name_from_filename(source_file_path, filename),
        'slug': transifex_name_from_filename(source_file_path, filename),
        'content': xml_content,
        'i18n_type': i18n_type
    }
    headers = {'Content-Type': 'application/json'}
    # r = requests.post(url, json=payload, auth=get_auth(), headers=headers)
    r = requests.post(url, json=payload, auth=get_auth(), headers=headers)
    if r.status_code < 200 or r.status_code > 299:
        if r.content.find(
                'Resource with this Slug and Project already exists.') != -1:
            return update_source_string_file_to_transifex(source_file_path,
                                                          filename,
                                                          xml_content)
        else:
            assert False, ('Aborting. Status code %d: %s' % (
                r.status_code, r.content))
    return True


def clean_triple_quoted_string(val):
    """Grit parses out first 3 and last 3 isngle quote chars if they exist."""
    val = val.strip()
    if val.startswith("'''"):
        val = val[3:]
    if val.endswith("'''"):
        val = val[:-3]
    return val.strip()


def textify(t):
    """Returns the text of a node to be translated"""
    val = lxml.etree.tostring(t, method='xml', encoding='unicode')
    val = val[val.index('>')+1:val.rindex('<')]
    val = clean_triple_quoted_string(val)
    return val


def replace_string_from_transifex(val):
    """Returns the text of a node from Transifex which also fixes up common
       problems that localizers do"""
    if val is None:
        return val
    val = (val.replace('&amp;lt;', '&lt;')
           .replace('&amp;gt;', '&gt;')
           .replace('&amp;amp;', '&amp;'))
    return val


def textify_from_transifex(t):
    """Returns the text of a node from Transifex which also fixes up common
       problems that localizers do"""
    return replace_string_from_transifex(textify(t))


def get_grd_message_string_tags(grd_file_path):
    """Obtains all message tags of the specified GRD file"""
    output_elements = []
    elements = lxml.etree.parse(grd_file_path).findall('//message')
    for element in elements:
        if element.tag == 'message':
            output_elements.append(element)
        else:
            assert False, ('Unexpected tag name %s' % element.tag)

    elements = lxml.etree.parse(grd_file_path).findall('.//part')
    for element in elements:
        grd_base_path = os.path.dirname(grd_file_path)
        grd_part_filename = element.get('file')
        if grd_part_filename in ['chromeos_strings.grdp']:
            continue
        grd_part_path = os.path.join(grd_base_path, grd_part_filename)
        part_output_elements = get_grd_message_string_tags(grd_part_path)
        output_elements.extend(part_output_elements)

    return output_elements


def get_fingerprint_for_xtb(message_tag):
    """Obtains the fingerprint meant for xtb files from a message tag."""
    string_to_hash = message_tag.text
    string_phs = message_tag.findall('ph')
    for string_ph in string_phs:
        string_to_hash = (
            (string_to_hash or '') + string_ph.get('name').upper() + (
                string_ph.tail or ''))
    string_to_hash = (string_to_hash or '').strip().encode('utf-8')
    string_to_hash = clean_triple_quoted_string(string_to_hash)
    fp = FP.FingerPrint(string_to_hash.decode('utf-8'))
    meaning = (message_tag.get('meaning') if 'meaning' in message_tag.attrib
               else None)
    if meaning:
        # combine the fingerprints of message and meaning
        fp2 = FP.FingerPrint(meaning)
        if fp < 0:
            fp = fp2 + (fp << 1) + 1
        else:
            fp = fp2 + (fp << 1)
    # To avoid negative ids we strip the high-order bit
    return str(fp & 0x7fffffffffffffffL)


def is_translateable_string(grd_file_path, message_tag):
    if message_tag.get('translateable') != 'false':
        return True
    # Check for exceptions that aren't translateable in Chromium, but are made
    # to be translateable in Brave. These can be found in the main function in
    # brave/script/chromium-rebase-l10n.py
    grd_file_name = os.path.basename(grd_file_path)
    if grd_file_name == 'chromium_strings.grd':
        exceptions = {'IDS_SXS_SHORTCUT_NAME',
                      'IDS_SHORTCUT_NAME_BETA',
                      'IDS_SHORTCUT_NAME_DEV',
                      'IDS_APP_SHORTCUTS_SUBDIR_NAME_BETA',
                      'IDS_APP_SHORTCUTS_SUBDIR_NAME_DEV',
                      'IDS_INBOUND_MDNS_RULE_NAME_BETA',
                      'IDS_INBOUND_MDNS_RULE_NAME_CANARY',
                      'IDS_INBOUND_MDNS_RULE_NAME_DEV',
                      'IDS_INBOUND_MDNS_RULE_DESCRIPTION_BETA',
                      'IDS_INBOUND_MDNS_RULE_DESCRIPTION_CANARY',
                      'IDS_INBOUND_MDNS_RULE_DESCRIPTION_DEV'}
        if message_tag.get('name') in exceptions:
            return True
    return False


def get_grd_strings(grd_file_path, validate_tags=True):
    """Obtains a tubple of (name, value, FP) for each string in a GRD file"""
    strings = []
    # Keep track of duplicate mesasge_names
    dupe_dict = defaultdict(int)
    all_message_tags = get_grd_message_string_tags(grd_file_path)
    for message_tag in all_message_tags:
        # Skip translateable="false" strings
        if not is_translateable_string(grd_file_path, message_tag):
            continue
        message_name = message_tag.get('name')
        dupe_dict[message_name] += 1

        # Check for a duplicate message_name, this can happen for example
        # for the same message id but one is title case and the other isn't.
        # Both need to be uploaded to Transifex with different message names.
        # When XTB files are later generated, the ID doesn't matter at all.
        # The only thing that matters is the fingerprint string hash.
        if dupe_dict[message_name] > 1:
            message_name += "_%s" % dupe_dict[message_name]
        if validate_tags:
            message_xml = lxml.etree.tostring(message_tag, method='xml', encoding='utf-8')
            errors = validate_tags_in_one_string(lxml.etree.fromstring(message_xml))
            assert errors is None, '\n' + errors
        message_desc = message_tag.get('desc') or ''
        message_value = textify(message_tag)
        assert message_name, 'Message name is empty'
        assert (message_name.startswith('IDS_') or
                message_name.startswith('PRINT_PREVIEW_MEDIA_')), (
                    'Invalid message ID: %s' % message_name)
        string_name = message_name[4:].lower()
        string_fp = get_fingerprint_for_xtb(message_tag)
        string_tuple = (string_name, message_value, string_fp, message_desc)
        strings.append(string_tuple)
    return strings


def get_json_strings(json_file_path):
    with open(json_file_path) as f:
        data = json.load(f)
    strings = []
    for key in data:
        string_name = key + '.message'
        string_value = data[key]["message"]
        string_desc = data[key]["description"] if "description" in data[key] else ""
        string_tuple = (string_name, string_value, string_desc)
        strings.append(string_tuple)
    return strings


def generate_source_strings_xml_from_grd(output_xml_file_handle,
                                         grd_file_path):
    """Generates a source string xml file from a GRD file"""
    resources_tag = create_android_format_resources_tag()
    all_strings = get_grd_strings(grd_file_path)
    for (string_name, string_value, _, _) in all_strings:
        resources_tag.append(
            create_android_format_string_tag(string_name, string_value))
    print 'Generating %d strings for GRD: %s' % (
        len(all_strings), grd_file_path)
    xml_string = lxml.etree.tostring(resources_tag)
    os.write(output_xml_file_handle, xml_string.encode('utf-8'))
    return xml_string


def check_plural_string_formatting(grd_string_content, translation_content):
    """Checks 'plurar' string formatting in translations"""
    pattern = re.compile(
        r"\s*{.*,\s*plural,(\s*offset:[0-2])?(\s*(=[0-2]|[zero|one|two|few|many])"
        r"\s*{(.*)})+\s*other\s*{(.*)}\s*}\s*")
    if pattern.match(grd_string_content) is not None:
        if pattern.match(translation_content) is None:
            error = ('Translation of plural string:\n'
                     '-----------\n{0}\n-----------\n'
                     'does not match:\n'
                     '-----------\n{1}\n-----------\n').format(
                         grd_string_content.encode('utf-8'),
                         translation_content.encode('utf-8'))
            raise ValueError(error)
    else:
        # This finds plural strings that the pattern above doesn't catch
        leading_pattern = re.compile(r"\s*{.*,\s*plural,.*")
        if leading_pattern.match(grd_string_content) != None:
            error = ('Uncaught plural pattern:\n'
                     '-----------\n{0}\n-----------\n').format(
                         grd_string_content.encode('utf-8'))
            raise ValueError(error)


def generate_xtb_content(lang_code, grd_strings, translations):
    """Generates an XTB file from a set of translations and GRD strings"""
    # Used to make sure duplicate fingerprint stringsa re not made
    # XTB only contains 1 entry even if multiple string names are
    # different but have the same value.
    all_string_fps = set()
    translationbundle_tag = create_xtb_format_translationbundle_tag(lang_code)
    for string in grd_strings:
        if string[0] in translations:
            fingerprint = string[2]
            if fingerprint in all_string_fps:
                continue
            all_string_fps.add(fingerprint)
            translation = translations[string[0]]
            if len(translation) != 0:
                check_plural_string_formatting(string[1], translation)
                translationbundle_tag.append(
                    create_xtb_format_translation_tag(
                        fingerprint, translation))

    xml_string = lxml.etree.tostring(translationbundle_tag)
    xml_string = HTMLParser.HTMLParser().unescape(
        xml_string.encode('utf-8')).encode('utf-8')
    xml_string = (
        '<?xml version="1.0" ?>\n<!DOCTYPE translationbundle>\n' + xml_string)
    return xml_string


def get_xtb_files(grd_file_path):
    """Obtains all the XTB filesi from the the specified GRD"""
    all_xtb_file_tags = (
        lxml.etree.parse(grd_file_path).findall('.//translations/file'))
    xtb_files = []
    for xtb_file_tag in all_xtb_file_tags:
        lang = xtb_file_tag.get('lang')
        path = xtb_file_tag.get('path')
        pair = (lang, path)
        xtb_files.append(pair)
    return xtb_files


def get_original_grd(src_root, grd_file_path):
    """Obtains the Chromium GRD file for a specified Brave GRD file."""
    # pylint: disable=fixme
    # TODO: consider passing this mapping into the script from l10nUtil.js
    grd_file_name = os.path.basename(grd_file_path)
    if grd_file_name == 'components_brave_strings.grd':
        return os.path.join(src_root, 'components',
                            'components_chromium_strings.grd')
    elif grd_file_name == 'brave_strings.grd':
        return os.path.join(src_root, 'chrome', 'app', 'chromium_strings.grd')
    elif grd_file_name == 'generated_resources.grd':
        return os.path.join(src_root, 'chrome', 'app',
                            'generated_resources.grd')
    elif grd_file_name == 'android_chrome_strings.grd':
        return os.path.join(src_root, 'chrome', 'browser', 'ui', 'android',
                            'strings', 'android_chrome_strings.grd')


def check_for_chromium_upgrade_extra_langs(src_root, grd_file_path):
    """Checks the Brave GRD file vs the Chromium GRD file for extra
    languages."""
    chromium_grd_file_path = get_original_grd(src_root, grd_file_path)
    if not chromium_grd_file_path:
        return
    brave_langs = get_transifex_languages(grd_file_path)
    chromium_langs = get_transifex_languages(chromium_grd_file_path)
    x_brave_extra_langs = brave_langs - chromium_langs
    assert len(x_brave_extra_langs) == 0, (
        'Brave GRD %s has extra languages %s over Chromium GRD %s' % (
            grd_file_path, chromium_grd_file_path,
            list(x_brave_extra_langs)))
    x_chromium_extra_langs = chromium_langs - brave_langs
    assert len(x_chromium_extra_langs) == 0, (
        'Chromium GRD %s has extra languages %s over Brave GRD %s' % (
            chromium_grd_file_path, grd_file_path,
            list(x_chromium_extra_langs)))


def get_transifex_string_hash(string_name):
    """Obains transifex string hash for the passed string."""
    key = string_name.encode('utf-8')
    return str(md5(':'.join([key, ''])).hexdigest())


def braveify(string_value):
    """Replace Chromium branded strings with Brave beranded strings."""
    return (string_value.replace('Chrome', 'Brave')
            .replace('Chromium', 'Brave')
            .replace('Google', 'Brave')
            .replace('Brave Docs', 'Google Docs')
            .replace('Brave Drive', 'Google Drive')
            .replace('Brave Play', 'Google Play')
            .replace('Brave Safe', 'Google Safe')
            .replace('Sends URLs of some pages you visit to Brave',
                     'Sends URLs of some pages you visit to Google')
            .replace('Brave Account', 'Brave sync chain'))


def upload_missing_translation_to_transifex(source_string_path, lang_code,
                                            filename, string_name,
                                            translated_value):
    """Uploads the specified string to the specified language code."""
    url_part = 'project/%s/resource/%s/translation/%s/string/%s/' % (
        transifex_project_name, transifex_name_from_filename(
            source_string_path, filename), lang_code,
        get_transifex_string_hash(string_name))
    url = base_url + url_part
    translated_value = braveify(translated_value)
    payload = {
        'translation': translated_value,
        # Assume Chromium provided strings are reviewed and proofread
        'reviewed': True,
        'proofread': True,
        'user': 'bbondy'
    }
    headers = {'Content-Type': 'application/json'}
    r = requests.put(url, json=payload, auth=get_auth(), headers=headers)
    assert r.status_code >= 200 and r.status_code <= 299, (
        'Aborting. Status code %d: %s' % (r.status_code, r.content))
    print 'Uploaded %s string: %s...' % (lang_code, string_name)
    return True


def upload_missing_json_translations_to_transifex(source_string_path):
    source_strings = get_json_strings(source_string_path)
    langs_dir_path = os.path.dirname(os.path.dirname(source_string_path))
    lang_codes = get_acceptable_json_lang_codes(langs_dir_path)
    filename = transifex_name_from_filename(source_string_path, '')
    for lang_code in lang_codes:
        l10n_path = os.path.join(langs_dir_path, lang_code, 'messages.json')
        l10n_strings = get_json_strings(l10n_path)
        l10n_dict = {string_name: string_value for (string_name, string_value, _) in l10n_strings}
        for (string_name, string_value, _) in source_strings:
            if string_name not in l10n_dict:
                # print 'Skipping string name %s for lang %s, not existing' % (
                #    string_name, lang_code)
                continue
            if l10n_dict[string_name] == string_value:
                # print 'Skipping string name %s for lang %s, not localized' % (
                #    string_name, lang_code)
                continue
            translation_value = (l10n_dict[string_name]
                                 .replace("\"", "\\\"")
                                 .replace("\r", "\\r")
                                 .replace("\n", "\\n"))
            upload_missing_translation_to_transifex(source_string_path,
                                                    lang_code, filename,
                                                    string_name.split(".")[0],
                                                    translation_value)


def check_for_chromium_upgrade(src_root, grd_file_path):
    """Performs various checks and changes as needed for when Chromium source
    files change."""
    check_for_chromium_upgrade_extra_langs(src_root, grd_file_path)


def get_transifex_source_resource_strings(grd_file_path):
    """Obtains the list of strings from Transifex"""
    filename = os.path.basename(grd_file_path).split('.')[0]
    url_part = (
        'project/%s/resource/%s/content/' % (
            transifex_project_name,
            transifex_name_from_filename(grd_file_path, filename)))
    url = base_url + url_part
    r = requests.get(url, auth=get_auth())
    assert r.status_code >= 200 and r.status_code <= 299, (
        'Aborting. Status code %d: %s' % (r.status_code, r.content))
    return get_strings_dict_from_xml_content(
        r.json()['content'].encode('utf-8'))


def check_missing_source_grd_strings_to_transifex(grd_file_path):
    """Compares the GRD strings to the strings on Transifex and uploads any
    missing strings."""
    source_grd_strings = get_grd_strings(grd_file_path)
    if len(source_grd_strings) == 0:
        return
    strings_dict = get_transifex_source_resource_strings(grd_file_path)
    transifex_string_ids = set(strings_dict.keys())
    grd_strings_tuple = get_grd_strings(grd_file_path)
    grd_string_names = {string_name for (string_name, message_value,
                                         string_fp, desc) in grd_strings_tuple}
    x_grd_extra_strings = grd_string_names - transifex_string_ids
    assert len(x_grd_extra_strings) == 0, (
        'GRD has extra strings over Transifex %s' %
        list(x_grd_extra_strings))
    x_transifex_extra_strings = transifex_string_ids - grd_string_names
    assert len(x_transifex_extra_strings) == 0, (
        'Transifex has extra strings over GRD %s' %
        list(x_transifex_extra_strings))


def upload_source_files_to_transifex(source_file_path, filename):
    uploaded = False
    i18n_type = ''
    content = ''
    ext = os.path.splitext(source_file_path)[1]
    if ext == '.grd':
        # Generate the intermediate Transifex format for the source
        # translations.
        temp_file = tempfile.mkstemp('.xml')
        output_xml_file_handle, _ = temp_file
        content = generate_source_strings_xml_from_grd(output_xml_file_handle,
                                                       source_file_path)
        os.close(output_xml_file_handle)
        i18n_type = 'ANDROID'
    elif ext == '.json':
        i18n_type = 'CHROME'
        with io.open(source_file_path, mode='r',
                     encoding='utf-8') as json_file:
            content = json_file.read()
    else:
        assert False, 'Unsupported source file ext %s: %s' % (
            ext, source_file_path)

    uploaded = upload_source_string_file_to_transifex(source_file_path,
                                                      filename, content,
                                                      i18n_type)
    assert uploaded, 'Could not upload xml file'


def get_acceptable_json_lang_codes(langs_dir_path):
    lang_codes = set(os.listdir(langs_dir_path))
    # Source language for Brave locales
    lang_codes.discard('en_US')

    # Source language for ethereum-remote-client
    lang_codes.discard('en')

    # Files that are not locales
    lang_codes.discard('.DS_Store')
    lang_codes.discard('index.json')

    # ethereum-remote-client has these unsupported locales
    lang_codes.discard('tml')
    lang_codes.discard('hn')
    lang_codes.discard('ph')
    lang_codes.discard('ht')
    return lang_codes


def pull_source_files_from_transifex(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    if ext == '.grd':
        # Generate the intermediate Transifex format
        xtb_files = get_xtb_files(source_file_path)
        base_path = os.path.dirname(source_file_path)
        grd_strings = get_grd_strings(source_file_path)
        for (lang_code, xtb_rel_path) in xtb_files:
            xtb_file_path = os.path.join(base_path, xtb_rel_path)
            print 'Updating: ', xtb_file_path, lang_code
            xml_content = get_transifex_translation_file_content(
                source_file_path, filename, lang_code)
            xml_content = fixup_bad_ph_tags_from_raw_transifex_string(xml_content)
            errors = validate_tags_in_transifex_strings(xml_content)
            assert errors is None, errors
            xml_content = trim_ph_tags_in_xtb_file_content(xml_content)
            translations = get_strings_dict_from_xml_content(xml_content)
            xtb_content = generate_xtb_content(lang_code, grd_strings,
                                               translations)
            with open(xtb_file_path, mode='w') as f:
                f.write(xtb_content)
    elif ext == '.json':
        langs_dir_path = os.path.dirname(os.path.dirname(source_file_path))
        lang_codes = get_acceptable_json_lang_codes(langs_dir_path)
        for lang_code in lang_codes:
            print 'getting filename %s for lang_code %s' % (filename,
                                                            lang_code)
            content = get_transifex_translation_file_content(source_file_path,
                                                             filename,
                                                             lang_code)
            localized_translation_path = (
                os.path.join(langs_dir_path, lang_code, 'messages.json'))
            dir_path = os.path.dirname(localized_translation_path)
            if not os.path.exists(dir_path):
                os.mkdir(dir_path)
            with open(localized_translation_path, mode='w') as f:
                f.write(content)


def upload_string_desc(source_file_path, filename, string_name, string_desc):
    """Uploads descriptions for strings"""
    url_part = 'project/%s/resource/%s/source/%s' % (
        transifex_project_name,
        transifex_name_from_filename(source_file_path, filename),
        get_transifex_string_hash(string_name))
    url = base_url + url_part
    payload = {
        'comment': string_desc,
    }
    print 'uploading string description for url: ', url
    headers = {'Content-Type': 'application/json'}
    r = requests.put(url, json=payload, auth=get_auth(), headers=headers)
    if r.status_code == 400 and 'Source string does not exist' in r.content:
        print 'Warning: Source string does not exist for: ', string_name
        return
    assert r.status_code >= 200 and r.status_code <= 299, (
        'Aborting. Status code %d: %s' % (r.status_code, r.content))


def upload_source_strings_desc(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    print 'Uploading strings descriptions for ', source_file_path
    if ext == '.json':
        json_strings = get_json_strings(source_file_path)
        for (string_name, _, string_desc) in json_strings:
            if len(string_desc) > 0:
                upload_string_desc(source_file_path, filename, string_name, string_desc)
    else:
        grd_strings = get_grd_strings(source_file_path)
        for (string_name, _, _, string_desc) in grd_strings:
            if len(string_desc) > 0:
                upload_string_desc(source_file_path, filename, string_name, string_desc)


def get_chromium_grd_src_with_fallback(grd_file_path, brave_source_root):
    source_root = os.path.dirname(brave_source_root)
    chromium_grd_file_path = get_original_grd(source_root, grd_file_path)
    if not chromium_grd_file_path:
        rel_path = os.path.relpath(grd_file_path, brave_source_root)
        chromium_grd_file_path = os.path.join(source_root, rel_path)
    return chromium_grd_file_path


def should_use_transifex(source_string_path, filename):
    slug = transifex_name_from_filename(source_string_path, filename)
    return slug in transifex_handled_slugs or slug.startswith('greaselion_')


def pull_xtb_without_transifex(grd_file_path, brave_source_root):
    xtb_files = get_xtb_files(grd_file_path)
    chromium_grd_file_path = get_chromium_grd_src_with_fallback(grd_file_path,
                                                                brave_source_root)
    chromium_xtb_files = get_xtb_files(grd_file_path)
    if len(xtb_files) != len(chromium_xtb_files):
        assert False, 'XTB files and Chromium XTB file length mismatch.'

    grd_base_path = os.path.dirname(grd_file_path)
    chromium_grd_base_path = os.path.dirname(chromium_grd_file_path)

    # Update XTB FPs so it uses the branded source string
    grd_strings = get_grd_strings(grd_file_path, validate_tags=False)
    chromium_grd_strings = get_grd_strings(chromium_grd_file_path, validate_tags=False)
    assert len(grd_strings) == len(chromium_grd_strings)

    fp_map = {chromium_grd_strings[idx][2]: grd_strings[idx][2] for
              (idx, grd_string) in enumerate(grd_strings)}

    xtb_file_paths = [os.path.join(
        grd_base_path, path) for (_, path) in xtb_files]
    chromium_xtb_file_paths = [
        os.path.join(chromium_grd_base_path, path) for
        (_, path) in chromium_xtb_files]
    for idx, xtb_file in enumerate(xtb_file_paths):
        chromium_xtb_file = chromium_xtb_file_paths[idx]
        if not os.path.exists(chromium_xtb_file):
            print 'Warning: Skipping because Chromium path does not exist: ', chromium_xtb_file
            continue
        xml_tree = lxml.etree.parse(chromium_xtb_file)

        for node in xml_tree.xpath('//translation'):
            generate_braveified_node(node, False, True)
            # Use our fp, when exists.
            old_fp = node.attrib['id']
            # It's possible for an xtb string to not be in our GRD.
            # This happens, for exmaple, with Chrome OS strings which
            # we don't process files for.
            if old_fp in fp_map:
                new_fp = fp_map.get(old_fp)
                if new_fp != old_fp:
                    node.attrib['id'] = new_fp
                    # print('fp: {0} -> {1}').format(old_fp, new_fp)

        transformed_content = ('<?xml version="1.0" ?>\n' +
                               lxml.etree.tostring(xml_tree, pretty_print=True,
                                                   xml_declaration=False,
                                                   encoding='UTF-8').strip())
        with open(xtb_file, mode='w') as f:
            f.write(transformed_content)


def combine_override_xtb_into_original(source_string_path):
    source_base_path = os.path.dirname(source_string_path)
    override_path = get_override_file_path(source_string_path)
    override_base_path = os.path.dirname(override_path)
    xtb_files = get_xtb_files(source_string_path)
    override_xtb_files = get_xtb_files(override_path)
    assert len(xtb_files) == len(override_xtb_files)

    for (idx, _) in enumerate(xtb_files):
        (lang, xtb_path) = xtb_files[idx]
        (override_lang, override_xtb_path) = override_xtb_files[idx]
        assert lang == override_lang

        xtb_tree = lxml.etree.parse(os.path.join(source_base_path, xtb_path))
        override_xtb_tree = lxml.etree.parse(os.path.join(override_base_path, override_xtb_path))
        translationbundle = xtb_tree.xpath('//translationbundle')[0]
        override_translations = override_xtb_tree.xpath('//translation')
        translations = xtb_tree.xpath('//translation')

        override_translation_fps = [t.attrib['id'] for t in override_translations]
        translation_fps = [t.attrib['id'] for t in translations]

        # Remove translations that we have a matching FP for
        for translation in xtb_tree.xpath('//translation'):
            if translation.attrib['id'] in override_translation_fps:
                translation.getparent().remove(translation)
            elif translation_fps.count(translation.attrib['id']) > 1:
                translation.getparent().remove(translation)
                translation_fps.remove(translation.attrib['id'])

        # Append the override translations into the original translation bundle
        for translation in override_translations:
            translationbundle.append(translation)

        xtb_content = ('<?xml version="1.0" ?>\n' +
                       lxml.etree.tostring(xtb_tree, pretty_print=True,
                                           xml_declaration=False,
                                           encoding='UTF-8').strip())
        with open(os.path.join(source_base_path, xtb_path), mode='w') as f:
            f.write(xtb_content)
