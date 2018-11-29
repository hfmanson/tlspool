#!/bin/sh

LD_PRELOAD=/usr/local/lib/libtlspool.so java -cp target/TlspoolSocket-1.0-SNAPSHOT.jar -Djava.library.path=target/classes nl.mansoft.tlspoolsocket.TlspoolSSLSocketFactory $@

