package nl.mansoft.tlspoolsocket;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.logging.Level;
import java.util.logging.Logger;

public class TlspoolSocket extends Socket {
	static {
		System.loadLibrary("tlspooljni");
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

    private native int startTls0();
    private native int readEncrypted(byte[] b, int off, int len) throws IOException;
    private native int writeEncrypted(byte[] b, int off, int len) throws IOException;

    private Socket socket;
    private PlainOutputStream pos;
    private PlainInputStream pis;
    private int plainfd;
    private int cryptfd;
    private Thread readEncryptedThread;
    private Thread writeEncryptedThread;

    public TlspoolSocket(Socket socket) {
        plainfd = -1;
        cryptfd = -1;
        this.socket = socket;
    }

    @Override
    public OutputStream getOutputStream() {
        return pos;
    }

    @Override
    public InputStream getInputStream() {
        return pis;
    }

    public void startTls() {
        readEncryptedThread = new Thread() {
            @Override
            public void run() {
                byte[] buf = new byte[4096];
                while (true) {
                    try {
                        System.out.println("readEncryptedThread: cryptfd = " + cryptfd);
                        if (cryptfd == -1) {
                            System.out.println("readEncryptedThread: sleeping for 0.1 second");
                            Thread.sleep(100);
                        } else {
                            int bytesRead = socket.getInputStream().read(buf);
                            System.out.println("readEncryptedThread: bytes read from socket: " + bytesRead);
                            if (bytesRead == -1) {
                                return;
                            }
                            writeEncrypted(buf, 0, bytesRead);
                        }
                    } catch (Exception ex) {
                        Logger.getLogger(TlspoolSocket.class.getName()).log(Level.SEVERE, null, ex);
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
                        System.out.println("writeEncryptedThread: cryptfd = " + cryptfd);
                        if (cryptfd == -1) {
                            System.out.println("writeEncryptedThread: sleeping for 0.1 second");
                            Thread.sleep(100);
                        } else {
                            int bytesRead = readEncrypted(buf, 0, buf.length);
                            if (bytesRead <= 0) {
                                return;
                            }
                            System.out.println("writeEncryptedThread, bytes read: " + bytesRead + ", sending to socket");
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
        startTls0();
        System.out.println("plainfd: " + plainfd);
        System.out.println("cryptfd: " + cryptfd);
        pos = new PlainOutputStream(plainfd);
        pis = new PlainInputStream(plainfd);
    }

    public void joinReadEncryptedThread() throws InterruptedException {
        readEncryptedThread.join();
    }

    public void joinWriteEncryptedThread() throws InterruptedException {
        writeEncryptedThread.join();
    }
}
