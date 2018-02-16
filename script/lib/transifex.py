import os
import requests
import sys
import xml.etree.ElementTree
import FP


transifex_organization_name = 'brave'
transifex_project_name = 'brave'
localization_format = 'ANDROID'
base_url = 'https://www.transifex.com/api/2/'


def create_xtb_format_translationbundle_tag():
  """Creates the root XTB XML element"""
  return xml.etree.ElementTree.Element('translationbundle')


def create_xtb_format_translation_tag(fingerprint, string):
  """Creates child XTB elements for each translation tag"""
  string_tag = xml.etree.ElementTree.Element('translation')
  string_tag.set('id', str(fingerprint))
  string_tag.text = string
  string_tag.tail = '\n'
  return string_tag


def create_android_format_resources_tag():
  """Creates intermediate Android format root tag"""
  return xml.etree.ElementTree.Element('resources')


def create_android_format_string_tag(string_name, string_value):
  """Creates intermediate Android format child tag for each translation string"""
  string_tag = xml.etree.ElementTree.Element('string')
  string_tag.set('name', string_name)
  string_tag.text = string_value
  string_tag.tail = '\n'
  return string_tag


def get_auth(username, password, transifex_api_key):
  """Creates an HTTPBasicAuth object given the Transifex information"""
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
  return [lang for (lang, xtb_rel_path) in xtb_files]


def get_transifex_translation_file_content(filename, lang_code, username, password, transifex_api_key):
  """Obtains a translation Android xml format and returns the string"""
  lang_code = lang_code.replace('-', '_')
  url_part = 'project/%s/resource/%s/translation/%s' % (transifex_project_name, filename, lang_code)
  url = base_url + url_part
  r = requests.get(url, auth=get_auth(username, password, transifex_api_key))
  if r.status_code < 200 or r.status_code > 299:
    sys.exit('Aborting. Status code %d: %s' % (r.status_code, r.content))
  return r.json()['content']


def get_strings_from_xml_content(xml_content):
  """Obtains a dictionary mapping the string name to text from an Android xml tag"""
  strings = xml.etree.ElementTree.fromstring(xml_content).findall('string')
  return { string_tag.get('name'): textify(string_tag) for string_tag in strings }


def upload_source_string_file_to_transifex(filename, xml_content, username, password, transifex_api_key):
  """Uploads the specified source string file to transifex"""
  url_part = 'project/%s/resources/' % transifex_project_name
  url = base_url + url_part
  api_key_username = "api:" + transifex_api_key
  payload = {
    'name': filename,
    'slug': filename,
    'content': xml_content,
    'i18n_type': localization_format
  }
  headers = { 'Content-Type': 'application/json' }
  r = requests.post(url, json=payload, auth=get_auth(username, password, transifex_api_key), headers=headers)
  if ((r.status_code < 200 or r.status_code > 299) and
          r.content.find('Resource with this Slug and Project already exists.') == -1):
    print 'Aborting. Status code %d: %s' % (r.status_code, r.content)
    return False
  return True


def textify(t):
  """Returns the text of a node to be translated"""
  s = []
  if t.text:
    s.append(t.text)
  for child in t.getchildren():
    #s.extend(textify(child))
    s.append(xml.etree.ElementTree.tostring(child))
  if t.tail:
    s.append(t.tail)
  return ''.join(s).strip()


def get_grd_message_string_tags(grd_file_path):
  """Obtains all message tags of the specified GRD file"""
  return xml.etree.ElementTree.parse(grd_file_path).findall('.//message')


def get_grd_strings(grd_file_path):
  """Obtains a tubple of (name, value, FP) for each string in a GRD file"""
  strings = []
  all_message_tags = get_grd_message_string_tags(grd_file_path)
  for message_tag in all_message_tags:
    message_name = message_tag.get('name')
    translateable = message_tag.get('translateable')
    if translateable == 'false':
      continue
    message_value = textify(message_tag)
    # print('message ID: ' + message_name + ', value: ' + message_value)
    if not message_name:
      sys.exit('Message name is empty')
    if not message_name.startswith('IDS_'):
      sys.exit('Invalid message ID: ' + message_name + ', value: ' + message_value)
    string_name = message_name[4:].lower()
    string_fp = FP.FingerPrint(message_value.encode('utf-8')) & 0x7fffffffffffffffL
    string_tuple = (string_name, message_value, string_fp)
    strings.append(string_tuple)
  return strings


def generate_source_strings_xml_from_grd(output_xml_file_handle, grd_file_path):
  """Generates a source string xml file from a GRD file"""
  resources_tag = create_android_format_resources_tag()
  all_strings = get_grd_strings(grd_file_path)
  for (string_name, string_value, fp) in all_strings:
    resources_tag.append(create_android_format_string_tag(string_name, string_value))
  print 'generating %d strings for GRD: %s' % (len(all_strings), grd_file_path)
  xml_string = xml.etree.ElementTree.tostring(resources_tag, 'utf-8')
  os.write(output_xml_file_handle, xml_string)
  return xml_string


def generate_xtb_content(grd_strings, translations):
  """Generates an XTB file from a set of translations and GRD strings"""
  translationbundle_tag = create_xtb_format_translationbundle_tag()
  for string in grd_strings:
    if string[0] in translations:
      fingerprint = string[2]
      translation = translations[string[0]]
      translationbundle_tag.append(create_xtb_format_translation_tag(fingerprint, translation))

  xml_string = xml.etree.ElementTree.tostring(translationbundle_tag, 'utf-8')
  return xml_string


def get_xtb_files(grd_file_path):
  """Obtains all the XTB filesi from the the specified GRD"""
  all_xtb_file_tags = xml.etree.ElementTree.parse(grd_file_path).findall('.//translations/file')
  xtb_files = []
  for xtb_file_tag in all_xtb_file_tags:
    lang = xtb_file_tag.get('lang')
    path = xtb_file_tag.get('path')
    pair = (lang, path)
    xtb_files.append(pair)
  return xtb_files
