package nl.mansoft.tlspoolsocket;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.net.ssl.HandshakeCompletedListener;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

public class TlspoolSocket extends SSLSocket {
    public static final int IPPROTO_TCP = 6;
    public static final int IPPROTO_UDP = 17;
    public static final int IPPROTO_SCTP = 132;

    public static final int PIOF_STARTTLS_LOCALROLE_CLIENT = 0x01;
    public static final int PIOF_STARTTLS_LOCALROLE_SERVER = 0x02;
    public static final int PIOF_STARTTLS_LOCALROLE_PEER = 0x03;
    public static final int PIOF_STARTTLS_REMOTEROLE_CLIENT = 0x04;
    public static final int PIOF_STARTTLS_REMOTEROLE_SERVER = 0x08;
    public static final int PIOF_STARTTLS_REMOTEROLE_PEER = 0x0c;
    public static final int PIOF_STARTTLS_BOTHROLES_PEER = 0x0f;

    static {
        System.loadLibrary("tlspooljni");
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
        return null;
    }

    @Override
    public void addHandshakeCompletedListener(HandshakeCompletedListener hl) {
    }

    @Override
    public void removeHandshakeCompletedListener(HandshakeCompletedListener hl) {
    }

    @Override
    public void startHandshake() throws IOException {
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

    private Socket socket;
    private PlainOutputStream pos;
    private PlainInputStream pis;
    private int plainfd;
    private int cryptfd;
    private Thread readEncryptedThread;
    private Thread writeEncryptedThread;
    private boolean autoClose;

    public TlspoolSocket(Socket socket, boolean autoClose) {
        plainfd = -1;
        cryptfd = -1;
        this.socket = socket;
        this.autoClose = autoClose;
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
        try {
            stopTls0();
            writeEncryptedThread.join();
            System.err.println("W joined");
            if (autoClose) {
                socket.close();
                readEncryptedThread.join();
                System.err.println("R joined");
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
    }
}
