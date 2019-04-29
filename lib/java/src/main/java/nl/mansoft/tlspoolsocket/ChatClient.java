package nl.mansoft.tlspoolsocket;

import java.io.OutputStream;
import java.net.Socket;
import java.security.Principal;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

public class ChatClient {
    private SSLSocket sslSocket;

    public void chat() throws Exception {
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
        String line;
        System.out.println("peer host: " + session.getPeerHost() + ", port: " + session.getPeerPort());
        RunTerminal runTerminal = new RunTerminal(sslSocket);
        runTerminal.start();
        while ((line = runTerminal.readLine()) != null && !line.isEmpty()) {
            line += "\n";
            os.write(line.getBytes());
        }
        sslSocket.close();
        System.out.println("EXITING");
    }

    public static void main(String[] args) throws Exception {
        new ChatClient().chat();
    }
}
