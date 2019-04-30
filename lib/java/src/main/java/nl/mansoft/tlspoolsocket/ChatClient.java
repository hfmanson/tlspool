package nl.mansoft.tlspoolsocket;

import java.io.OutputStream;
import java.net.Socket;
import java.security.Principal;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

public class ChatClient {
    private SSLSocket sslSocket;

    public void chat(String host, String remoteid, int port) throws Exception {
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
        if (args.length != 3) {
            System.err.println("Usage: ChatClient <remoteid> <remote ip> <port>");
        }
        String remoteid = args[0];
        String host = args[1];
        int port = Integer.parseInt(args[2]);
        new ChatClient().chat(host, remoteid, port);
    }
}
