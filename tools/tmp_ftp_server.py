#!/usr/bin/env python3
"""
Temporary FTP server for one-off uploads.
User: nalle
Password: uglemat64
Home dir: ~/ftp_upload
Port: 2121

This script will attempt to print helpful logs and run until interrupted (Ctrl+C).
"""
import os
import logging
from pathlib import Path

# Try to import pyftpdlib; if missing, print a helpful message and exit.
try:
    from pyftpdlib.authorizers import DummyAuthorizer
    from pyftpdlib.handlers import FTPHandler
    from pyftpdlib.servers import FTPServer
except Exception as e:
    print("pyftpdlib is not installed. Install with: pip3 install --user pyftpdlib")
    raise

HOME = Path.home()
FTP_HOME = HOME / 'ftp_upload'
FTP_HOME.mkdir(mode=0o755, exist_ok=True)

# configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
log = logging.getLogger('tmp_ftp_server')

user = 'nalle'
password = 'uglemat64'

authorizer = DummyAuthorizer()
# full perms for convenience; adjust if you want read-only etc.
authorizer.add_user(user, password, str(FTP_HOME), perm='elradfmwMT')
# optionally allow anonymous read-only (disabled)
# authorizer.add_anonymous(str(FTP_HOME), perm='elr')

handler = FTPHandler
handler.authorizer = authorizer
handler.banner = 'Temporary FTP server. Upload logs here.'

address = ('0.0.0.0', 2121)
server = FTPServer(address, handler)

log.info('Starting temporary FTP server')
log.info('User: %s    Password: %s', user, password)
log.info('Home dir: %s', FTP_HOME)
log.info('Listening on %s:%d', address[0], address[1])

try:
    server.serve_forever()
except KeyboardInterrupt:
    log.info('Shutting down FTP server')
    server.close_all()
except Exception:
    log.exception('Server error, shutting down')
    server.close_all()
