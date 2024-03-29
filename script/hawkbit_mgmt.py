#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
# SPDX-FileCopyrightText: 2021 Enrico Jörns <e.joerns@pengutronix.de>, Pengutronix
# SPDX-FileCopyrightText: 2021 Bastian Krause <bst@pengutronix.de>, Pengutronix

import time

import attr
import requests as r


class HawkbitError(Exception):
    pass

class HawkbitIdStore(dict):
    """dict raising a HawkbitMgmtTestClient related error on KeyError."""
    def __getitem__(self, key):
        try:
            return super().__getitem__(key)
        except KeyError:
            raise HawkbitError(f'{key} not yet created via HawkbitMgmtTestClient')

@attr.s(eq=False)
class HawkbitMgmtTestClient:
    """
    Test oriented client for hawkBit's Management API.
    Does not cover the whole Management API, only the parts required for the rauc-hawkbit-updater
    test suite.

    https://eclipse.dev/hawkbit/apis/management_api/
    """
    host = attr.ib(validator=attr.validators.instance_of(str))
    port = attr.ib(validator=attr.validators.instance_of(int))
    username = attr.ib(default='admin', validator=attr.validators.instance_of(str))
    password = attr.ib(default='admin', validator=attr.validators.instance_of(str))
    version = attr.ib(default=1.0, validator=attr.validators.instance_of(float))

    def __attrs_post_init__(self):
        self.url = f'http://{self.host}:{self.port}/rest/v1/{{endpoint}}'
        self.id = HawkbitIdStore()

    def get(self, endpoint: str):
        """
        Performs an authenticated HTTP GET request on `endpoint`.
        Endpoint can either be a full URL or a path relative to /rest/v1/. Expects and returns the
        JSON response.
        """
        url = endpoint if endpoint.startswith('http') else self.url.format(endpoint=endpoint)
        req = r.get(
            url,
            headers={'Content-Type': 'application/json;charset=UTF-8'},
            auth=(self.username, self.password)
        )
        if req.status_code != 200:
            try:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.json()}')
            except:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.content.decode()}')

        return req.json()

    def post(self, endpoint: str, json_data: dict = None, file_name: str = None):
        """
        Performs an authenticated HTTP POST request on `endpoint`.
        If `json_data` is given, it is sent along with the request and JSON data is expected in the
        response, which is in that case returned.
        If `file_name` is given, the file's content is sent along with the request and JSON data is
        expected in the response, which is in that case returned.
        json_data and file_name must not be specified in the same call.
        Endpoint can either be a full URL or a path relative to /rest/v1/.
        """
        assert not (json_data and file_name)

        url = endpoint if endpoint.startswith('http') else self.url.format(endpoint=endpoint)
        files = {'file': open(file_name, 'rb')} if file_name else None
        headers = {'Content-Type': 'application/json;charset=UTF-8'} if json_data else None

        req = r.post(
            url,
            headers=headers,
            auth=(self.username, self.password),
            json=json_data,
            files=files
        )

        if not 200 <= req.status_code < 300:
            try:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.json()}')
            except:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.content.decode()}')

        if json_data or file_name:
            return req.json()

        return None

    def put(self, endpoint: str, json_data: dict):
        """
        Performs an authenticated HTTP PUT request on `endpoint`. `json_data` is sent along with
        the request.
        `endpoint` can either be a full URL or a path relative to /rest/v1/.
        """
        url = endpoint if endpoint.startswith('http') else self.url.format(endpoint=endpoint)

        req = r.put(
            url,
            auth=(self.username, self.password),
            json=json_data
        )
        if not 200 <= req.status_code < 300:
            try:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.json()}')
            except:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.content.decode()}')

    def delete(self, endpoint: str):
        """
        Performs an authenticated HTTP DELETE request on endpoint.
        Endpoint can either be a full URL or a path relative to /rest/v1/.
        """
        url = endpoint if endpoint.startswith('http') else self.url.format(endpoint=endpoint)

        req = r.delete(
            url,
            auth=(self.username, self.password)
        )
        if not 200 <= req.status_code < 300:
            try:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.json()}')
            except:
                raise HawkbitError(f'HTTP error {req.status_code}: {req.content.decode()}')

    def set_config(self, key: str, value: str):
        """
        Changes a configuration `value` of a specific configuration `key`.

        https://eclipse.dev/hawkbit/rest-api/tenant-api-guide.html#_put_restv1systemconfigskeyname
        """
        self.put(f'system/configs/{key}', {'value' : value})

    def get_config(self, key: str):
        """
        Returns the configuration value of a specific configuration `key`.

        https://eclipse.dev/hawkbit/rest-api/tenant-api-guide.html#_get_restv1systemconfigskeyname
        """
        return self.get(f'system/configs/{key}')['value']

    def add_target(self, target_id: str = None, token: str = None):
        """
        Adds a new target with id and name `target_id`.
        If `target_id` is not given, a generic id is made up.
        If `token` is given, set it as target's token, otherwise hawkBit sets a random token
        itself.
        Stores the id of the created target for future use by other methods.
        Returns the target's id.

        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_post_restv1targets
        """
        target_id = target_id or f'test-{time.monotonic()}'
        testdata = {
            'controllerId': target_id,
            'name': target_id,
        }

        if token:
            testdata['securityToken'] = token

        self.post('targets', [testdata])

        self.id['target'] = target_id
        return self.id['target']

    def get_target(self, target_id: str = None):
        """
        Returns the target matching `target_id`.
        If `target_id` is not given, returns the target created by the most recent `add_target()`
        call.

        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_get_restv1targetstargetid
        """
        target_id = target_id or self.id['target']

        return self.get(f'targets/{target_id}')

    def delete_target(self, target_id: str = None):
        """
        Deletes the target matching `target_id`.
        If target_id is not given, deletes the target created by the most recent add_target() call.

        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_delete_restv1targetstargetid
        """
        target_id = target_id or self.id['target']
        self.delete(f'targets/{target_id}')

        if 'target' in self.id and target_id == self.id['target']:
            del self.id['target']

    def get_attributes(self, target_id: str = None):
        """
        Returns the attributes of the target matching `target_id`.
        If `target_id` is not given, uses the target created by the most recent `add_target()`
        call.
        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_get_restv1targetstargetidattributes
        """
        target_id = target_id or self.id['target']

        return self.get(f'targets/{target_id}/attributes')

    def add_softwaremodule(self, name: str = None, module_type: str = 'os'):
        """
        Adds a new software module with `name`.
        If `name` is not given, a generic name is made up.
        Stores the id of the created software module for future use by other methods.
        Returns the id of the created software module.

        https://eclipse.dev/hawkbit/rest-api/softwaremodules-api-guide.html#_post_restv1softwaremodules
        """
        name = name or f'software module {time.monotonic()}'
        data = [{
            'name': name,
            'version': str(self.version),
            'type': module_type,
        }]

        self.id['softwaremodule'] = self.post('softwaremodules', data)[0]['id']
        return self.id['softwaremodule']

    def get_softwaremodule(self, module_id: str = None):
        """
        Returns the sotware module matching `module_id`.
        If `module_id` is not given, returns the software module created by the most recent
        `add_softwaremodule()` call.

        https://eclipse.dev/hawkbit/rest-api/softwaremodules-api-guide.html#_get_restv1softwaremodulessoftwaremoduleid
        """
        module_id = module_id or self.id['softwaremodule']

        return self.get(f'softwaremodules/{module_id}')

    def delete_softwaremodule(self, module_id: str = None):
        """
        Deletes the software module matching `module_id`.

        https://eclipse.dev/hawkbit/rest-api/softwaremodules-api-guide.html#_delete_restv1softwaremodulessoftwaremoduleid
        """
        module_id = module_id or self.id['softwaremodule']
        self.delete(f'softwaremodules/{module_id}')

        if 'softwaremodule' in self.id and module_id == self.id['softwaremodule']:
            del self.id['softwaremodule']

    def add_distributionset(self, name: str = None, module_ids: list = [], dist_type: str = 'os'):
        """
        Adds a new distribution set with `name` containing the software modules matching `module_ids`.
        If `name` is not given, a generic name is made up.
        If `module_ids` is not given, uses the software module created by the most recent
        `add_softwaremodule()` call.
        Stores the id of the created distribution set for future use by other methods.
        Returns the id of the created distribution set.

        https://eclipse.dev/hawkbit/rest-api/distributionsets-api-guide.html#_post_restv1distributionsets
        """
        assert isinstance(module_ids, list)

        name = name or f'distribution {self.version} ({time.monotonic()})'
        module_ids = module_ids or [self.id['softwaremodule']]
        data = [{
            'name': name,
            'description': 'Test distribution',
            'version': str(self.version),
            'modules': [],
            'type': dist_type,
        }]
        for module_id in module_ids:
            data[0]['modules'].append({'id': module_id})

        self.id['distributionset'] = self.post('distributionsets', data)[0]['id']
        return self.id['distributionset']

    def get_distributionset(self, dist_id: str = None):
        """
        Returns the distribution set matching `dist_id`.
        If `dist_id` is not given, returns the distribution set created by the most recent
        `add_distributionset()` call.

        https://eclipse.dev/hawkbit/rest-api/distributionsets-api-guide.html#_get_restv1distributionsetsdistributionsetid
        """
        dist_id = dist_id or self.id['distributionset']

        return self.get(f'distributionsets/{dist_id}')

    def delete_distributionset(self, dist_id: str = None):
        """
        Deletes the distrubition set matching `dist_id`.
        If `dist_id` is not given, deletes the distribution set created by the most recent
        `add_distributionset()` call.

        https://eclipse.dev/hawkbit/rest-api/distributionsets-api-guide.html#_delete_restv1distributionsetsdistributionsetid
        """
        dist_id = dist_id or self.id['distributionset']

        self.delete(f'distributionsets/{dist_id}')

        if 'distributionset' in self.id and dist_id == self.id['distributionset']:
            del self.id['distributionset']

    def add_artifact(self, file_name: str, module_id: str = None):
        """
        Adds a new artifact specified by `file_name` to the software module matching `module_id`.
        If `module_id` is not given, adds the artifact to the software module created by the most
        recent `add_softwaremodule()` call.
        Stores the id of the created artifact for future use by other methods.
        Returns the id of the created artifact.

        https://eclipse.dev/hawkbit/rest-api/softwaremodules-api-guide.html#_post_restv1softwaremodulessoftwaremoduleidartifacts
        """
        module_id = module_id or self.id['softwaremodule']

        self.id['artifact'] = self.post(f'softwaremodules/{module_id}/artifacts',
                                        file_name=file_name)['id']
        return self.id['artifact']

    def get_artifact(self, artifact_id: str = None, module_id: str = None):
        """
        Returns the artifact matching `artifact_id` from the software module matching `module_id`.
        If `artifact_id` is not given, returns the artifact created by the most recent
        `add_artifact()` call.
        If `module_id` is not given, uses the software module created by the most recent
        `add_softwaremodule()` call.

        https://eclipse.dev/hawkbit/rest-api/softwaremodules-api-guide.html#_get_restv1softwaremodulessoftwaremoduleidartifactsartifactid
        """
        module_id = module_id or self.id['softwaremodule']
        artifact_id = artifact_id or self.id['artifact']

        return self.get(f'softwaremodules/{module_id}/artifacts/{artifact_id}')['id']

    def delete_artifact(self, artifact_id: str = None, module_id: str = None):
        """
        Deletes the artifact matching `artifact_id` from the software module matching `module_id`.
        If `artifact_id` is not given, deletes the artifact created by the most recent
        `add_artifact()` call.
        If `module_id` is not given, uses the software module created by the most recent
        `add_softwaremodule()` call.

        https://eclipse.dev/hawkbit/rest-api/softwaremodules-api-guide.html#_delete_restv1softwaremodulessoftwaremoduleidartifactsartifactid
        """
        module_id = module_id or self.id['softwaremodule']
        artifact_id = artifact_id or self.id['artifact']

        self.delete(f'softwaremodules/{module_id}/artifacts/{artifact_id}')

        if 'artifact' in self.id and artifact_id == self.id['artifact']:
            del self.id['artifact']

    def assign_target(self, dist_id: str = None, target_id: str = None, params: dict = None):
        """
        Assigns the distribution set matching `dist_id` to a target matching `target_id`.
        If `dist_id` is not given, uses the distribution set created by the most recent
        `add_distributionset()` call.
        If `target_id` is not given, uses the target created by the most recent `add_target()`
        call.
        Stores the id of the assignment action for future use by other methods.

        https://eclipse.dev/hawkbit/rest-api/distributionsets-api-guide.html#_post_restv1distributionsetsdistributionsetidassignedtargets
        """
        dist_id = dist_id or self.id['distributionset']
        target_id = target_id or self.id['target']
        testdata = [{'id': target_id}]

        if params:
            testdata[0].update(params)

        response = self.post(f'distributionsets/{dist_id}/assignedTargets', testdata)

        # Increment version to be able to flash over an already deployed distribution
        self.version += 0.1

        self.id['action'] = response.get('assignedActions')[-1].get('id')
        return self.id['action']

    def get_action(self, action_id: str = None, target_id: str = None):
        """
        Returns the action matching `action_id` on the target matching `target_id`.
        If `action_id` is not given, returns the action created by the most recent
        `assign_target()` call.
        If `target_id` is not given, uses the target created by the most recent `add_target()`
        call.

        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_get_restv1targetstargetidactionsactionid
        """
        action_id = action_id or self.id['action']
        target_id = target_id or self.id['target']

        return self.get(f'targets/{target_id}/actions/{action_id}')

    def get_action_status(self, action_id: str = None, target_id: str = None):
        """
        Returns the first (max.) 50 action states of the action matching `action_id` of the target
        matching `target_id` sorted by id.
        If `action_id` is not given, uses the action created by the most recent `assign_target()`
        call.
        If `target_id` is not given, uses the target created by the most recent `add_target()`
        call.

        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_get_restv1targetstargetidactionsactionidstatus
        """
        action_id = action_id or self.id['action']
        target_id = target_id or self.id['target']

        req = self.get(f'targets/{target_id}/actions/{action_id}/status?offset=0&limit=50&sort=id:DESC')
        return req['content']

    def cancel_action(self, action_id: str = None, target_id: str = None, *, force: bool = False):
        """
        Cancels the action matching `action_id` of the target matching `target_id`.
        If `force=True` is given, cancels the action without telling the target.
        If `action_id` is not given, uses the action created by the most recent `assign_target()`
        call.
        If `target_id` is not given, uses the target created by the most recent `add_target()`
        call.

        https://eclipse.dev/hawkbit/rest-api/targets-api-guide.html#_delete_restv1targetstargetidactionsactionid
        """
        action_id = action_id or self.id['action']
        target_id = target_id or self.id['target']

        self.delete(f'targets/{target_id}/actions/{action_id}')

        if force:
            self.delete(f'targets/{target_id}/actions/{action_id}?force=true')


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('bundle', help='RAUC bundle to add as artifact')
    args = parser.parse_args()

    client = HawkbitMgmtTestClient('localhost', 8080)

    client.set_config('pollingTime', '00:00:30')
    client.set_config('pollingOverdueTime', '00:03:00')
    client.set_config('authentication.targettoken.enabled', True)

    client.add_target('test', 'ieHai3du7gee7aPhojeth4ong')
    client.add_softwaremodule()
    client.add_artifact(args.bundle)
    client.add_distributionset()
    client.assign_target()

    try:
        target = client.get_target()
        print(f'Created target (target_name={target["controllerId"]}, auth_token={target["securityToken"]}) assigned distribution containing {args.bundle} to it')
        print('Clean quit with a single ctrl-c')

        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print('Cleaning up..')
    finally:
        try:
            client.cancel_action(force=True)
        except:
            pass

        client.delete_distributionset()
        client.delete_artifact()
        client.delete_softwaremodule()
        client.delete_target()
        print('Done')
