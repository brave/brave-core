import base64
import mimetypes
import requests


def get_content_type(filename):
    return mimetypes.guess_type(filename)[0] or 'application/octet-stream'


def get(url, headers):
    response = requests.get(url, headers=headers)
    return response


def post(url, params, headers):
    response = requests.post(url, json=params, headers=headers)
    return response


def post_with_file(url, files, params, headers):
    response = requests.post(url, files=files, data=params, headers=headers)
    return response
