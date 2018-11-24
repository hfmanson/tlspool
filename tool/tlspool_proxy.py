#!/usr/bin/env python
#
# protocolHandler for "web+tlspool" URI scheme.
#
# This can be registered with some browsers, as described on
# https://developer.mozilla.org/en-US/docs/Web/API/Navigator/registerProtocolHandler
#
# The intention is to redirect URIs of our own design, like
#    web+tlspool://orvelte.nep/bakkerij
# into
#    https://some.server/?uri=web+tlspool://orvelte.nep/bakkerij
# with some escapes in the URI, of course.
#
# Since some.server can have a plain server certificate, this is
# more straightforward than using an HTTP or HTTPS proxy, for which
# we would need on-the-fly re-signing with a CA that the browser
# must accept.
#
# From: Rick van Rein <rick@openfortress.nl>


import os
import sys
import re
import time
import urllib

import socket
import tlspool

from SocketServer import ThreadingMixIn
from BaseHTTPServer import HTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler


path_re = re.compile ('^http:\/\/(.*?)(?::([\d]+))?(\/.*)$')

class ThreadingServer(ThreadingMixIn, HTTPServer):
    pass

class RequestHandler(SimpleHTTPRequestHandler):
    def do_tlspool(self, host, port, path):
        print 'DEBUG: host to connect to is: %s' % host
        # sox = socket.socket (socket.AF_INET6, socket.SOCK_STREAM)
        sox = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        sox.connect ( (host, int (port or '443')) )
        cnx = tlspool.Connection (cryptsocket=sox)
        cnx.tlsdata.flags = ( tlspool.PIOF_STARTTLS_LOCALROLE_CLIENT |
                              tlspool.PIOF_STARTTLS_REMOTEROLE_SERVER |
                              tlspool.PIOF_STARTTLS_FORK |
                              tlspool.PIOF_STARTTLS_DETACH )
        cnx.tlsdata.remoteid = host
        cnx.tlsdata.ipproto = socket.IPPROTO_TCP
        cnx.tlsdata.service = 'http'
        try:
                sox = cnx.starttls ()
        except:
                self.send_response (403, 'Forbidden')
                return
        req = self.command + ' ' + path + ' ' + self.request_version + '\r\n'
        print 'DEBUG: %s' % req
        sox.send (req)
        sox.send (str (self.headers) + '\r\n')

        data = sox.recv (4096)
        while data != '':
                self.wfile.write (data)
                data = sox.recv (4096)

    def do_GET(self):
        print 'DEBUG: Request path is %s' % (str (self.path),)
        uri = path_re.match (self.path)
        if uri is None:
            self.send_response (400, 'Bad Request')
            return
        groups = uri.groups ()
        print 'DEBUG: groups: %s' % (str (groups),)
        host = groups [0]
        port = groups [1]
        if port is None:
            port = 80
        else:
            port = int(port)
        path = groups [2]
        if port % 1000 == 443:
            self.do_tlspool(host, port, path)
        else:
            self.copyfile(urllib.urlopen(self.path), self.wfile)

ThreadingServer(('', 8080), RequestHandler).serve_forever()
