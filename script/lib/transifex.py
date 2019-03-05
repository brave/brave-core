from hashlib import md5
from lib.config import get_env_var
from xml.sax.saxutils import escape, unescape
from collections import defaultdict
import HTMLParser
import io
import json
import os
import requests
import sys
import tempfile
import lxml.etree
import FP


transifex_project_name = 'brave'
base_url = 'https://www.transifex.com/api/2/'


def transifex_name_from_filename(source_file_path, filename):
    ext = os.path.splitext(source_file_path)[1]
    if 'brave_components_strings' in source_file_path:
        return 'brave_components_resources'
    elif ext == '.grd':
        return filename
    elif 'brave_extension' in source_file_path:
        return 'brave_extension'
    elif 'brave_rewards' in source_file_path:
        return 'rewards_extension'
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
        'lang', lang.replace('_', '-').replace('he', 'iw'))
    # Adds a newline so the first translation isn't glued to the
    # translationbundle element for us weak humans.
    translationbundle_tag.text = '\n'
    return translationbundle_tag


def create_xtb_format_translation_tag(fingerprint, string_value):
    """Creates child XTB elements for each translation tag"""
    string_tag = lxml.etree.Element('translation')
    string_tag.set('id', str(fingerprint))
    if string_value.count('<') != string_value.count('>'):
        assert False, 'Warning: Unmatched < character, consider fixing on '
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
    return set([lang for (lang, xtb_rel_path) in xtb_files])


def get_transifex_translation_file_content(source_file_path, filename,
                                           lang_code):
    """Obtains a translation Android xml format and returns the string"""
    lang_code = lang_code.replace('-', '_')
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
        content = content.replace('\\"', '"').replace("\\'", "'")
        # Make sure it's parseable
        lxml.etree.fromstring(content)
    return content


def get_strings_dict_from_xml_content(xml_content):
    """Obtains a dictionary mapping the string name to text from Android xml
    content"""
    strings = lxml.etree.fromstring(xml_content).findall('string')
    return {string_tag.get('name'): textify(string_tag)
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
        if grd_part_filename in ['chromeos_strings.grdp',
                                 'media_router_resources.grdp']:
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
            string_to_hash + string_ph.get('name').upper() + (
                string_ph.tail or ''))
    string_to_hash = (string_to_hash or '').strip().encode('utf-8')
    string_to_hash = clean_triple_quoted_string(string_to_hash)
    fp = FP.FingerPrint(string_to_hash)
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


def get_grd_strings(grd_file_path):
    """Obtains a tubple of (name, value, FP) for each string in a GRD file"""
    strings = []
    # Keep track of duplicate mesasge_names
    dupe_dict = defaultdict(int)
    all_message_tags = get_grd_message_string_tags(grd_file_path)
    for message_tag in all_message_tags:
        message_name = message_tag.get('name')
        dupe_dict[message_name] += 1

        # Check for a duplicate message_name, this can happen for example
        # for the same message id but one is title case and the other isn't.
        # Both need to be uploaded to Transifex with different message names.
        # When XTB files are later generated, the ID doesn't matter at all.
        # The only thing that matters is the fingerprint string hash.
        if dupe_dict[message_name] > 1:
            message_name += "_%s" % dupe_dict[message_name]
        message_desc = message_tag.get('desc') or ''
        message_value = textify(message_tag)
        assert not not message_name, 'Message name is empty'
        assert message_name.startswith('IDS_'), (
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
        string_desc = data[key]["description"]
        string_tuple = (string_name, string_value, string_desc)
        strings.append(string_tuple)
    return strings


def generate_source_strings_xml_from_grd(output_xml_file_handle,
                                         grd_file_path):
    """Generates a source string xml file from a GRD file"""
    resources_tag = create_android_format_resources_tag()
    all_strings = get_grd_strings(grd_file_path)
    for (string_name, string_value, fp, desc) in all_strings:
        resources_tag.append(
            create_android_format_string_tag(string_name, string_value))
    print 'Generating %d strings for GRD: %s' % (
        len(all_strings), grd_file_path)
    xml_string = lxml.etree.tostring(resources_tag)
    os.write(output_xml_file_handle, xml_string.encode('utf-8'))
    return xml_string


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
    grd_file_name = os.path.basename(grd_file_path)
    if grd_file_name == 'components_brave_strings.grd':
        return os.path.join(src_root, 'components',
                            'components_chromium_strings.grd')
    elif grd_file_name == 'brave_strings.grd':
        return os.path.join(src_root, 'chrome', 'app', 'chromium_strings.grd')
    elif grd_file_name == 'generated_resources.grd':
        return os.path.join(src_root, 'chrome', 'app',
                            'generated_resources.grd')


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


def check_for_chromium_missing_grd_strings(src_root, grd_file_path):
    """Checks to make sure Brave GRD file vs the Chromium GRD has the same
    amount of strings."""
    chromium_grd_file_path = get_original_grd(src_root, grd_file_path)
    if not chromium_grd_file_path:
        return
    grd_strings = get_grd_strings(grd_file_path)
    chromium_grd_strings = get_grd_strings(chromium_grd_file_path)
    brave_strings = {string_name for (
        string_name, message_value, string_fp, desc) in grd_strings}
    chromium_strings = {string_name for (
        string_name, message_value, string_fp, desc) in chromium_grd_strings}
    x_brave_extra_strings = brave_strings - chromium_strings
    assert len(x_brave_extra_strings) == 0, (
        'Brave GRD %s has extra strings %s over Chromium GRD %s' % (
            grd_file_path, chromium_grd_file_path,
            list(x_brave_extra_strings)))
    x_chromium_extra_strings = chromium_strings - brave_strings
    assert len(x_chromium_extra_strings) == 0, (
        'Chromium GRD %s has extra strings %s over Brave GRD %s' % (
            chromium_grd_file_path, grd_file_path,
            list(x_chromium_extra_strings)))


def get_transifex_string_hash(string_name):
    """Obains transifex string hash for the passed string."""
    key = string_name.encode('utf-8')
    return str(md5(':'.join([key, ''])).hexdigest())


def braveify(string_value):
    """Replace Chromium branded strings with Brave beranded strings."""
    return (string_value.replace('Chrome', 'Brave')
            .replace('Chromium', 'Brave')
            .replace('Google', 'Brave Software'))


def upload_missing_translation_to_transifex(source_string_path, lang_code,
                                            filename, string_name,
                                            string_value, translated_value):
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
    print 'Uploaded %s string: %s -- %s...' % (
        lang_code, string_name, translated_value[:12].encode('utf-8'))
    return True


def upload_missing_translations_to_transifex(source_string_path, lang_code,
                                             filename, grd_strings,
                                             chromium_grd_strings, xtb_strings,
                                             chromium_xtb_strings):
    """For each chromium translation that we don't know about, upload it."""
    lang_code = lang_code.replace('-', '_')
    for idx, (string_name, string_value,
              string_fp, desc) in enumerate(grd_strings):
        string_fp = str(string_fp)
        chromium_string_fp = str(chromium_grd_strings[idx][2])
        if chromium_string_fp in chromium_xtb_strings and (
                string_fp not in xtb_strings):
            # print 'Uploading for locale %s for missing '
            # 'string ID: %s' % (lang_code, string_name)
            upload_missing_translation_to_transifex(
                source_string_path, lang_code, filename, string_name,
                string_value, chromium_xtb_strings[chromium_string_fp])


def fix_missing_xtb_strings_from_chromium_xtb_strings(
        src_root, grd_file_path):
    """Checks to make sure Brave GRD file vs the Chromium GRD has the same
    amount of strings.  If they do this checks that the XTB files that we
    manage have all the equivalent strings as the Chromium XTB files."""
    chromium_grd_file_path = get_original_grd(src_root, grd_file_path)
    if not chromium_grd_file_path:
        return

    grd_base_path = os.path.dirname(grd_file_path)
    chromium_grd_base_path = os.path.dirname(chromium_grd_file_path)

    filename = os.path.basename(grd_file_path).split('.')[0]
    grd_strings = get_grd_strings(grd_file_path)
    chromium_grd_strings = get_grd_strings(chromium_grd_file_path)

    # Get the XTB files from each of the GRD files
    xtb_files = get_xtb_files(grd_file_path)
    chromium_xtb_files = get_xtb_files(chromium_grd_file_path)
    xtb_file_paths = [os.path.join(
        grd_base_path, path) for (lang, path) in xtb_files]
    chromium_xtb_file_paths = [
        os.path.join(chromium_grd_base_path, path) for
        (lang, path) in chromium_xtb_files]

    # langs is the same sized list as xtb_files but contains only the associated
    # list of locales.
    langs = [lang for (lang, path) in xtb_files]

    for idx, xtb_file in enumerate(xtb_file_paths):
        chromium_xtb_file = chromium_xtb_file_paths[idx]
        lang_code = langs[idx]
        xtb_strings = get_strings_dict_from_xtb_file(xtb_file)
        chromium_xtb_strings = get_strings_dict_from_xtb_file(
            chromium_xtb_file)
        assert(len(grd_strings) == len(chromium_grd_strings))
        upload_missing_translations_to_transifex(
            grd_file_path, lang_code, filename, grd_strings,
            chromium_grd_strings, xtb_strings, chromium_xtb_strings)


def check_for_chromium_upgrade(src_root, grd_file_path):
    """Performs various checks and changes as needed for when Chromium source
    files change."""
    check_for_chromium_upgrade_extra_langs(src_root, grd_file_path)
    check_for_chromium_missing_grd_strings(src_root, grd_file_path)
    fix_missing_xtb_strings_from_chromium_xtb_strings(src_root, grd_file_path)


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
    filename = os.path.basename(grd_file_path).split('.')[0]
    x_grd_extra_strings = grd_string_names - transifex_string_ids
    assert len(x_grd_extra_strings) == 0, (
        'GRD has extra strings over Transifex %' %
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
        output_xml_file_handle, output_xml_path = temp_file
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
            translations = get_strings_dict_from_xml_content(xml_content)
            xtb_content = generate_xtb_content(lang_code, grd_strings,
                                               translations)
            with open(xtb_file_path, mode='w') as f:
                f.write(xtb_content)
    elif ext == '.json':
        langs_dir_path = os.path.dirname(os.path.dirname(source_file_path))
        lang_codes = set(os.listdir(langs_dir_path))
        lang_codes.discard('en_US')
        lang_codes.discard('.DS_Store')
        for lang_code in lang_codes:
            print 'getting filename %s for lang_code %s' % (filename,
                                                            lang_code)
            content = get_transifex_translation_file_content(source_file_path,
                                                             filename,
                                                             lang_code)
            localized_translation_path = os.path.join(langs_dir_path,
                                                      lang_code,
                                                      'messages.json')
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
        for (string_name, string_value, string_desc) in json_strings:
            if len(string_desc) > 0:
                upload_string_desc(source_file_path, filename, string_name,
                                   string_desc)
    else:
        grd_strings = get_grd_strings(source_file_path)
        for (string_name, string_value, string_fp, string_desc) in grd_strings:
            if len(string_desc) > 0:
                upload_string_desc(source_file_path, filename, string_name,
                                   string_desc)
