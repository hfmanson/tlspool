package nl.mansoft.tlspoolsocket;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.security.Principal;
import java.security.cert.Certificate;
import java.util.HashSet;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.net.ssl.HandshakeCompletedEvent;
import javax.net.ssl.HandshakeCompletedListener;
import javax.net.ssl.SSLPeerUnverifiedException;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSessionContext;
import javax.net.ssl.SSLSocket;
import javax.security.auth.x500.X500Principal;
import javax.security.cert.X509Certificate;

public class TlspoolSocket extends SSLSocket implements SSLSession {
    public static final int IPPROTO_TCP = 6;
    public static final int IPPROTO_UDP = 17;
    public static final int IPPROTO_SCTP = 132;

    public static final int PIOF_STARTTLS_LOCALROLE_CLIENT  = 0x01;
    public static final int PIOF_STARTTLS_LOCALROLE_SERVER  = 0x02;
    public static final int PIOF_STARTTLS_LOCALROLE_PEER    = 0x03;
    public static final int PIOF_STARTTLS_REMOTEROLE_CLIENT = 0x04;
    public static final int PIOF_STARTTLS_REMOTEROLE_SERVER = 0x08;
    public static final int PIOF_STARTTLS_REMOTEROLE_PEER   = 0x0c;
    public static final int PIOF_STARTTLS_BOTHROLES_PEER    = 0x0f;
    public static final int PIOK_INFO_PEERCERT_SUBJECT		= 0x52800000;
    public static final int PIOK_INFO_PEERCERT_ISSUER		= 0x52800001;
    public static final int PIOK_INFO_MYCERT_SUBJECT		= 0x52800100;
    public static final int PIOK_INFO_MYCERT_ISSUER			= 0x52800101;

    static {
        System.loadLibrary("libtlspooljni");
    }

    @Override
    public String[] getSupportedCipherSuites() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public String[] getEnabledCipherSuites() {
        return null;
    }

    @Override
    public void setEnabledCipherSuites(String[] strings) {
    }

    @Override
    public String[] getSupportedProtocols() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public String[] getEnabledProtocols() {
        return null;
    }

    @Override
    public void setEnabledProtocols(String[] strings) {
    }

    @Override
    public SSLSession getSession() {
        return this;
    }

    @Override
    public void addHandshakeCompletedListener(HandshakeCompletedListener handshakeCompletedListener) {
        if (handshakeCompletedListener == null) {
            throw new IllegalArgumentException("handshakeCompletedListener is null");
        }
        listeners.add(handshakeCompletedListener);
    }

    @Override
    public void removeHandshakeCompletedListener(HandshakeCompletedListener handshakeCompletedListener) {
        if (handshakeCompletedListener == null || !listeners.remove(handshakeCompletedListener)) {
            throw new IllegalArgumentException("handshakeCompletedListener is null");
        }
    }

    @Override
    public void startHandshake() throws IOException {
        startTls(
            TlspoolSocket.PIOF_STARTTLS_LOCALROLE_CLIENT | TlspoolSocket.PIOF_STARTTLS_REMOTEROLE_SERVER,
            0,
            TlspoolSocket.IPPROTO_TCP,
            0,
            "testcli@tlspool.arpa2.lab",
            host,
            "generic",
            0
        );
        HandshakeCompletedEvent handshakeCompletedEvent = new HandshakeCompletedEvent(this, this);
        for (HandshakeCompletedListener handshakeCompletedListener : listeners) {
            handshakeCompletedListener.handshakeCompleted(handshakeCompletedEvent);
        }
    }

    @Override
    public void setUseClientMode(boolean bln) {
        System.err.println("setUseClientMode: " + bln);
    }

    @Override
    public boolean getUseClientMode() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void setNeedClientAuth(boolean bln) {
    }

    @Override
    public boolean getNeedClientAuth() {
        return false;
    }

    @Override
    public void setWantClientAuth(boolean bln) {
    }

    @Override
    public boolean getWantClientAuth() {
        return false;
    }

    @Override
    public void setEnableSessionCreation(boolean bln) {
        System.err.println("setEnableSessionCreation: " + bln);
    }

    @Override
    public boolean getEnableSessionCreation() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public byte[] getId() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public SSLSessionContext getSessionContext() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public long getCreationTime() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public long getLastAccessedTime() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void invalidate() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean isValid() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void putValue(String string, Object o) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Object getValue(String string) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void removeValue(String string) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public String[] getValueNames() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Certificate[] getPeerCertificates() throws SSLPeerUnverifiedException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Certificate[] getLocalCertificates() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public X509Certificate[] getPeerCertificateChain() throws SSLPeerUnverifiedException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Principal getPeerPrincipal() throws SSLPeerUnverifiedException {
        return getInfo(PIOK_INFO_PEERCERT_SUBJECT);
    }

    @Override
    public Principal getLocalPrincipal() {
        return getInfo(PIOK_INFO_MYCERT_SUBJECT);
    }

    @Override
    public String getCipherSuite() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public String getProtocol() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public String getPeerHost() {
        return host;
    }

    @Override
    public int getPeerPort() {
        return port;
    }

    @Override
    public int getPacketBufferSize() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public int getApplicationBufferSize() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    private static class PlainOutputStream extends OutputStream {
        private native void writePlain(byte[] b, int off, int len) throws IOException;
        private final int fd;

        public PlainOutputStream(int fd) {
            this.fd = fd;
        }

        @Override
        public void write(int b) throws IOException {
            byte[] barr = new byte[] { (byte) b };
            write(barr, 0, 1);
        }

        @Override
        public void write(byte[] b, int off, int len) throws IOException {
            writePlain(b, off, len);
        }
    }

    private static class PlainInputStream extends InputStream {
        private native int readPlain(byte[] b, int off, int len) throws IOException;
        private final int fd;

        public PlainInputStream(int fd) {
            this.fd = fd;
        }

        @Override
        public int read() throws IOException {
            byte[] barr = new byte[1];
            return read(barr, 0, 1);
        }

        @Override
        public int read(byte[] b, int off, int len) throws IOException {
            return readPlain(b, off, len);
        }
    }

    private native int startTls0(int flags, int local, int ipproto, int streamid, String localid, String remoteid, String service, int timeout);
    private native int stopTls0();
    private native int readEncrypted(byte[] b, int off, int len) throws IOException;
    private native int writeEncrypted(byte[] b, int off, int len) throws IOException;
    private native int shutdownWriteEncrypted() throws IOException;
    private native X500Principal getInfo(int kind);

    private Socket socket;
    private String host;
    private int port;
    private boolean autoClose;
    private PlainOutputStream pos;
    private PlainInputStream pis;
    private int plainfd;
    private int cryptfd;
    private Thread readEncryptedThread;
    private Thread writeEncryptedThread;
    private byte[] controlKey;
    private Set<HandshakeCompletedListener> listeners;
    public TlspoolSocket(Socket socket, String host, int port, boolean autoClose) {
        plainfd = -1;
        cryptfd = -1;
        this.socket = socket;
        this.host = host;
        this.port = port;
        this.autoClose = autoClose;
        listeners = new HashSet<HandshakeCompletedListener>();
    }

    @Override
    public OutputStream getOutputStream() {
        return pos;
    }

    @Override
    public InputStream getInputStream() {
        return pis;
    }

    @Override
    public void close() throws IOException {

		System.err.println("TlspoolSocket.close()");
		try {
			stopTls0();
			readEncryptedThread.join();
			System.err.println("R joined");
			writeEncryptedThread.join();
			System.err.println("W joined");
			if (autoClose) {
				socket.close();
			}
		} catch (InterruptedException ex) {
		}
    }

    public void startTls(int flags, int local, int ipproto, int streamid, String localid, String remoteid, String service, int timeout) {
        readEncryptedThread = new Thread() {
            @Override
            public void run() {
                byte[] buf = new byte[4096];
                while (true) {
                    try {
                        System.err.println("readEncryptedThread: cryptfd = " + cryptfd);
                        if (cryptfd == -1) {
                            System.err.println("readEncryptedThread: sleeping for 0.1 second");
                            Thread.sleep(100);
                        } else {
                            int bytesRead = socket.getInputStream().read(buf);
                            System.err.println("readEncryptedThread: bytes read from socket: " + bytesRead);
                            if (bytesRead == -1) {
                                shutdownWriteEncrypted();
                                System.err.println("EXIT: readEncryptedThread");
                                return;
                            }
                            writeEncrypted(buf, 0, bytesRead);
                        }
                    } catch (Exception ex) {
//                        Logger.getLogger(TlspoolSocket.class.getName()).log(Level.SEVERE, null, ex);
                        System.err.println(ex.getMessage());
                        return;
                    }
                }
            }
        };
        writeEncryptedThread = new Thread() {
            @Override
            public void run() {
                byte[] buf = new byte[4096];
                while (true) {
                    try {
                        System.err.println("writeEncryptedThread: cryptfd = " + cryptfd);
                        if (cryptfd == -1) {
                            System.err.println("writeEncryptedThread: sleeping for 0.1 second");
                            Thread.sleep(100);
                        } else {
                            int bytesRead = readEncrypted(buf, 0, buf.length);
                            if (bytesRead <= 0) {
                                System.err.println("EXIT: writeEncryptedThread");
                                return;
                            }
                            System.err.println("writeEncryptedThread, bytes read: " + bytesRead + ", sending to socket");
                            socket.getOutputStream().write(buf, 0, bytesRead);
                        }
                    } catch (Exception ex) {
                        Logger.getLogger(TlspoolSocket.class.getName()).log(Level.SEVERE, null, ex);
                        return;
                    }
                }
            }
        };
        readEncryptedThread.start();
        writeEncryptedThread.start();
        startTls0(flags, local, ipproto, streamid, localid, remoteid, service, timeout);
        System.err.println("plainfd: " + plainfd);
        System.err.println("cryptfd: " + cryptfd);
        pos = new PlainOutputStream(plainfd);
        pis = new PlainInputStream(plainfd);
        if (controlKey == null) {
            System.err.println("control key is null");
        }
    }
}
