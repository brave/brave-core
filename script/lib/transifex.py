import os
import requests
import sys
import xml.etree.ElementTree


transifex_project_name = 'brave'
localization_format = 'ANDROID'
base_url = 'https://www.transifex.com/api/2/'


def create_resources_tag():
  return xml.etree.ElementTree.Element('resources')


def create_string_tag(string_name, string_value):
  string_tag = xml.etree.ElementTree.Element('string')
  string_tag.set('name', string_name)
  string_tag.text = string_value
  string_tag.tail = '\n'
  return string_tag


def upload_string_file_to_transifex(filename, xml_content, username, password, transifex_api_key):
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

  auth = None
  if transifex_api_key:
    auth=requests.auth.HTTPBasicAuth(api_key_username, '')
  else:
    r = auth=requests.auth.HTTPBasicAuth(username, password)

  r = requests.post(url, json=payload, auth=auth, headers=headers)
  if ((r.status_code < 200 or r.status_code > 299) and
          r.content.find('Resource with this Slug and Project already exists.') == -1):
    print 'Aborting. Status code %d: %s' % (r.status_code, r.content)
    return False
  return True


def textify(t):
  s = []
  if t.text:
    s.append(t.text)
  for child in t.getchildren():
    #s.extend(textify(child))
    s.append(xml.etree.ElementTree.tostring(child))
  if t.tail:
    s.append(t.tail)
  return ''.join(s).strip()


def generate_strings_xml_from_grd(output_xml_file_handle, grd_path):
  all_messages = xml.etree.ElementTree.parse(grd_path).findall('.//message')
  resources_tag = create_resources_tag()
  for message_tag in all_messages:
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
    resources_tag.append(create_string_tag(string_name, message_value))
  print 'generating %d strings for GRD: %s' % (len(all_messages), grd_path)
  xml_string = xml.etree.ElementTree.tostring(resources_tag, 'utf-8')
  os.write(output_xml_file_handle, xml_string)
  return xml_string

