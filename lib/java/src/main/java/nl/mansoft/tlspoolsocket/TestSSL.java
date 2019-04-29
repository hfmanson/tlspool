package nl.mansoft.tlspoolsocket;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.security.Principal;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

public class TestSSL {
    private SSLSocket sslSocket;
    private ConsoleInputReadTask consoleInputReadTask;
    private final Thread socketThread = new Thread() {
        @Override
        public void run() {
            InputStream is = null;
            try {
                byte[] buf = new byte[4096];
                is = sslSocket.getInputStream();
                System.err.println("START: socketThread");
                while (true) {
                    try {
                        int bytesRead = is.read(buf);
                        System.err.println("socketThread: bytes read from socket: " + bytesRead);
                        if (bytesRead <= 0) {
                            System.err.println("EXIT: socketThread");
                            consoleInputReadTask.stop();
                            return;
                        }
                        System.out.println(new String(buf, 0, bytesRead));
                    } catch (Exception ex) {
                        System.err.println(ex.getMessage());
                        return;
                    }
                }
            } catch (IOException ex) {
                Logger.getLogger(TestSSL.class.getName()).log(Level.SEVERE, null, ex);
            } finally {
                try {
                    is.close();
                } catch (IOException ex) {
                    Logger.getLogger(TestSSL.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    };

    public String readLine() throws InterruptedException {
        ExecutorService ex = Executors.newSingleThreadExecutor();
        String input = null;
        try {
            Future<String> result = ex.submit(consoleInputReadTask);
            try {
                input = result.get();
            } catch (ExecutionException e) {
                e.getCause().printStackTrace();
            }
        } finally {
            ex.shutdownNow();
        }
        return input;
    }

    public void testSSL() throws Exception {
        String host = "localhost";
        String remoteid = "testsrv@tlspool.arpa2.lab";
        int port = 12345;
        Socket socket = new Socket(host, port);
        SSLSocketFactory factory = new TlspoolSSLSocketFactory();
        sslSocket = (SSLSocket) factory.createSocket(socket, remoteid, port, true);
        sslSocket.startHandshake();
        SSLSession session = sslSocket.getSession();
        Principal localsubject = session.getLocalPrincipal();
        System.err.println("local subject: " + localsubject);
        Principal peersubject = session.getPeerPrincipal();
        System.err.println("peer subject: " + peersubject);

        OutputStream os = sslSocket.getOutputStream();
        socketThread.start();
        String line;
        consoleInputReadTask = new ConsoleInputReadTask();
        System.out.println("peer host: " + session.getPeerHost() + ", port: " + session.getPeerPort());
        //BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        //while ((line = reader.readLine()) != null && line.charAt(0) != '.') {
        while ((line = readLine()) != null && !line.isEmpty()) {
            line += "\n";
            os.write(line.getBytes());
        }
        sslSocket.close();
        System.out.println("EXITING");
    }

    public static void main(String[] args) throws Exception {
        new TestSSL().testSSL();
    }
}

