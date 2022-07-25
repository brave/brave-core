#!/usr/bin/env python

from __future__ import print_function
from __future__ import absolute_import
from builtins import str
from builtins import object
import json
import os
import re
import requests
import sys
import base64
try:
    from .util import execute, scoped_cwd
except ImportError:
    pass

REQUESTS_DIR = os.path.abspath(os.path.join(__file__, '..', '..', '..',
                                            'vendor', 'requests'))
sys.path.append(os.path.join(REQUESTS_DIR, 'build', 'lib'))
sys.path.append(os.path.join(REQUESTS_DIR, 'build', 'lib.linux-x86_64-2.7'))

GITHUB_URL = 'https://api.github.com'
GITHUB_UPLOAD_ASSET_URL = 'https://uploads.github.com'


class GitHub(object):
    def __init__(self, access_token):
        self._authorization = 'token %s' % access_token

        pattern = '^/repos/{0}/{0}/releases/{1}/assets$'.format(
            '[^/]+', '[0-9]+')
        self._releases_upload_api_pattern = re.compile(pattern)

    def __getattr__(self, attr):
        return _Callable(self, '/%s' % attr)

    def send(self, method, path, **kw):
        if 'headers' not in kw:
            kw['headers'] = dict()
        headers = kw['headers']
        headers['Authorization'] = self._authorization
        headers['Accept'] = 'application/vnd.github.manifold-preview'

        # Switch to a different domain for the releases uploading API.
        if self._releases_upload_api_pattern.match(path):
            url = '%s%s' % (GITHUB_UPLOAD_ASSET_URL, path)
        else:
            url = '%s%s' % (GITHUB_URL, path)
            # Data are sent in JSON format.
            if 'data' in kw:
                kw['data'] = json.dumps(kw['data'])

        try:
            r = getattr(requests, method)(url, **kw).json()
        except ValueError:
            # Returned response may be empty in some cases
            r = {}
        if 'message' in r:
            raise Exception(json.dumps(r, indent=2, separators=(',', ': ')))
        return r


class _Executable(object):
    def __init__(self, gh, method, path):
        self._gh = gh
        self._method = method
        self._path = path

    def __call__(self, **kw):
        return self._gh.send(self._method, self._path, **kw)


class _Callable(object):
    def __init__(self, gh, name):
        self._gh = gh
        self._name = name

    def __call__(self, *args):
        if len(args) == 0:
            return self

        name = '%s/%s' % (self._name, '/'.join([str(arg) for arg in args]))
        return _Callable(self._gh, name)

    def __getattr__(self, attr):
        if attr in ['get', 'put', 'post', 'patch', 'delete']:
            return _Executable(self._gh, attr, self._name)

        name = '%s/%s' % (self._name, attr)
        return _Callable(self._gh, name)


def get_authenticated_user_login(token):
    """given a valid GitHub access token, return the associated GitHub user login"""
    # for more info see: https://developer.github.com/v3/users/#get-the-authenticated-user
    user = GitHub(token).user()
    try:
        response = user.get()
        return response['login']
    except Exception as e:
        print('[ERROR] ' + str(e))


def parse_user_logins(token, login_csv, verbose=False):
    """given a list of logins in csv format, parse into a list and validate logins"""
    if login_csv is None:
        return []
    login_csv = login_csv.replace(" ", "")
    parsed_logins = login_csv.split(',')

    users = GitHub(token).users()

    invalid_logins = []

    # check login/username against GitHub
    # for more info see: https://developer.github.com/v3/users/#get-a-single-user
    for login in parsed_logins:
        try:
            response = users(login).get()
            if verbose:
                print('[INFO] Login "' + login + '" found: ' + str(response))
        except Exception as e:
            if verbose:
                print('[INFO] Login "' + login +
                      '" does not appear to be valid. ' + str(e))
            invalid_logins.append(login)

    if len(invalid_logins) > 0:
        raise Exception(
            'Invalid logins found. Are they misspelled? ' + ','.join(invalid_logins))

    return parsed_logins


def parse_labels(token, repo_name, label_csv, verbose=False):
    global config
    if label_csv is None:
        return []
    label_csv = label_csv.replace(" ", "")
    parsed_labels = label_csv.split(',')

    invalid_labels = []

    # validate labels passed in are correct
    # for more info see: https://developer.github.com/v3/issues/labels/#get-a-single-label
    repo = GitHub(token).repos(repo_name)
    for label in parsed_labels:
        try:
            response = repo.labels(label).get()
            if verbose:
                print('[INFO] Label "' + label + '" found: ' + str(response))
        except Exception as e:
            if verbose:
                print('[INFO] Label "' + label +
                      '" does not appear to be valid. ' + str(e))
            invalid_labels.append(label)

    if len(invalid_labels) > 0:
        raise Exception(
            'Invalid labels found. Are they misspelled? ' + ','.join(invalid_labels))

    return parsed_labels


def get_file_contents(token, repo_name, filename, branch=None):
    # NOTE: API only supports files up to 1MB in size
    # for more info see: https://developer.github.com/v3/repos/contents/
    repo = GitHub(token).repos(repo_name)
    get_data = {}
    if branch:
        get_data['ref'] = branch
    file = repo.contents(filename).get(params=get_data)
    if file['encoding'] == 'base64':
        return base64.b64decode(file['content'])
    return file['content']


def add_reviewers_to_pull_request(token, repo_name, pr_number, reviewers=[], team_reviewers=[],
                                  verbose=False, dryrun=False):
    # add reviewers to pull request
    # for more info see: https://developer.github.com/v3/pulls/review_requests/
    repo = GitHub(token).repos(repo_name)
    patch_data = {}
    if len(reviewers) > 0:
        patch_data['reviewers'] = reviewers
    if len(team_reviewers) > 0:
        patch_data['team_reviewers'] = team_reviewers
    if dryrun:
        print('[INFO] would call `repo.pulls(' + str(pr_number) +
              ').requested_reviewers.post(' + str(patch_data) + ')`')
        return
    response = repo.pulls(pr_number).requested_reviewers.post(data=patch_data)
    if verbose:
        print('repo.pulls(' + str(pr_number) +
              ').requested_reviewers.post(data) response:\n' + str(response))
    return response


def get_milestones(token, repo_name, verbose=False):
    # get all milestones for a repo
    # for more info see: https://developer.github.com/v3/issues/milestones/
    repo = GitHub(token).repos(repo_name)
    response = repo.milestones.get()
    if verbose:
        print('repo.milestones.get() response:\n' + str(response))
    return response


def create_pull_request(token, repo_name, title, body, branch_src, branch_dst,
                        open_in_browser=False, verbose=False, dryrun=False):
    post_data = {
        'title': title,
        'head': branch_src,
        'base': branch_dst,
        'body': body,
        'maintainer_can_modify': True
    }
    # create the pull request
    # for more info see: http://developer.github.com/v3/pulls
    if dryrun:
        print('[INFO] would call `repo.pulls.post(' + str(post_data) + ')`')
        if open_in_browser:
            print('[INFO] would open PR in web browser')
        return 1234
    repo = GitHub(token).repos(repo_name)
    response = repo.pulls.post(data=post_data)
    if verbose:
        print('repo.pulls.post(data) response:\n' + str(response))
    if open_in_browser:
        import webbrowser
        webbrowser.open(response['html_url'])
    return int(response['number'])


def set_issue_details(token, repo_name, issue_number, milestone_number=None,
                      assignees=[], labels=[], verbose=False, dryrun=False):
    patch_data = {}
    if milestone_number:
        patch_data['milestone'] = milestone_number
    if len(assignees) > 0:
        patch_data['assignees'] = assignees
    if len(labels) > 0:
        patch_data['labels'] = labels
    # TODO: error if no keys in patch_data

    # add milestone and assignee to issue / pull request
    # for more info see: https://developer.github.com/v3/issues/#edit-an-issue
    if dryrun:
        print('[INFO] would call `repo.issues(' +
              str(issue_number) + ').patch(' + str(patch_data) + ')`')
        return
    repo = GitHub(token).repos(repo_name)
    response = repo.issues(issue_number).patch(data=patch_data)
    if verbose:
        print('repo.issues(' + str(issue_number) +
              ').patch(data) response:\n' + str(response))


def fetch_origin_check_staged(path):
    """given a path on disk (to a git repo), fetch origin and ensure there aren't unstaged files"""
    with scoped_cwd(path):
        execute(['git', 'fetch', 'origin'])
        status = execute(['git', 'status', '-s']).strip()
        if len(status) > 0:
            print('[ERROR] There appear to be unstaged changes.\n' +
                  'Please resolve these before running (ex: `git status`).')
            return 1
    return 0


def get_local_branch_name(path):
    with scoped_cwd(path):
        return execute(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip()


def get_title_from_first_commit(path, branch_to_compare):
    """get the first commit subject (useful for the title of a pull request)"""
    with scoped_cwd(path):
        local_branch = execute(
            ['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip()
        title_list = execute(['git', 'log', 'origin/' + branch_to_compare +
                             '..HEAD', '--pretty=format:%s', '--reverse'])
        title_list = title_list.split('\n')
        if len(title_list) == 0:
            raise Exception(
                'No commits found! Local branch matches "' + branch_to_compare + '"')
        return title_list[0]


def push_branches_to_remote(path, branches_to_push, dryrun=False, token=None):
    if dryrun:
        print('[INFO] would push the following local branches to remote: ' +
              str(branches_to_push))
    else:
        with scoped_cwd(path):
            for branch_to_push in branches_to_push:
                print('- pushing ' + branch_to_push + '...')
                # TODO: if they already exist, force push?? or error??
                response = execute(
                    ['git', 'remote', 'get-url', '--push', 'origin']).strip()
                if response.startswith('https://'):
                    if len(str(token)) == 0:
                        raise Exception(
                            'GitHub token cannot be null or empty!')
                    remote = response.replace(
                        'https://', 'https://' + token + ':x-oauth-basic@')
                    execute(['git', 'push', '-u', remote, branch_to_push])
                else:
                    execute(['git', 'push', '-u', 'origin', branch_to_push])
