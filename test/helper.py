# SPDX-License-Identifier: LGPL-2.1-only
# SPDX-FileCopyrightText: 2021 Enrico Jörns <e.joerns@pengutronix.de>, Pengutronix
# SPDX-FileCopyrightText: 2021-2022 Bastian Krause <bst@pengutronix.de>, Pengutronix

import os
import subprocess
import shlex
import logging
import socket
from contextlib import closing


class PExpectLogger:
    """
    pexpect Logger, allows to use Python's logging stdlib. To be passed as pexpect 'logfile".
    Logs linewise to given logger at given level.
    """
    def __init__(self, level=logging.INFO, logger=None):
        self.level = level
        self.data = b''
        self.logger = logger or logging.getLogger()

    def write(self, data):
        self.data += data

    def flush(self):
        for line in self.data.splitlines():
            self.logger.log(self.level, line.decode())

        self.data = b''

def logger_from_command(command):
    """
    Returns a logger named after the executable, or in case of a python executable, after the
    python module,
    """
    cmd_parts = command.split()
    base_cmd = os.path.basename(cmd_parts[0])
    try:
        if base_cmd.startswith('python') and cmd_parts[1] == '-m':
            base_cmd = command.split()[2]
    except IndexError:
        pass

    return logging.getLogger(base_cmd)

def run_pexpect(command, *, timeout=30, cwd=None):
    """
    Runs given command via pexpect with DBUS_STARTER_BUS_TYPE=session and PATH+=./build. Returns
    process handle immediately allowing further expect calls. Logs command and its
    stdout/stderr/exit code.
    """
    import pexpect
    logger = logger_from_command(command)
    logger.info('running: %s', command)

    pexpect_log = PExpectLogger(logger=logger)
    return pexpect.spawn(command, timeout=timeout, cwd=cwd, logfile=pexpect_log)

def run(command, *, timeout=30):
    """
    Runs given command as subprocess with DBUS_STARTER_BUS_TYPE=session and PATH+=./build. Blocks
    until command terminates. Logs command and its stdout/stderr/exit code.
    Returns tuple (stdout, stderr, exit code).
    """
    logger = logger_from_command(command)
    logger.info('running: %s', command)

    proc = subprocess.run(shlex.split(command), capture_output=True, text=True, check=False,
                          timeout=timeout)

    for line in proc.stdout.splitlines():
        if line:
            logger.info('stdout: %s', line)
    for line in proc.stderr.splitlines():
        if line:
            logger.warning('stderr: %s', line)

    logger.info('exitcode: %d', proc.returncode)

    return proc.stdout, proc.stderr, proc.returncode

def available_port():
    """Returns an available local port."""
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        sock.bind(('localhost', 0))
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        return sock.getsockname()[1]

def timezone_offset_utc(date):
    utc_offset = int(date.astimezone().utcoffset().total_seconds())

    utc_offset_hours, remainder = divmod(utc_offset, 60*60)
    utc_offset_minutes, remainder = divmod(remainder, 60)
    sign_offset = '+' if utc_offset >=0 else '-'

    if remainder != 0:
        raise Exception('UTC offset contains fraction of a minute')

    return f'{sign_offset}{utc_offset_hours:02}:{utc_offset_minutes:02}'
