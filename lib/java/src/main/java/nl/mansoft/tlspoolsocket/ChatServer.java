/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package nl.mansoft.tlspoolsocket;

import java.io.OutputStream;
import java.security.Principal;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLServerSocketFactory;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

/**
 *
 * @author hfman
 */
public class ChatServer {
    public void chat(int port) throws Exception {
        SSLServerSocketFactory factory = new TlspoolSSLServerSocketFactory();
        SSLServerSocket sslServerSocket = (SSLServerSocket) factory.createServerSocket(port);
        String line;
        do {
            SSLSocket sslSocket = (SSLSocket) sslServerSocket.accept();
            sslSocket.startHandshake();
            SSLSession session = sslSocket.getSession();
            Principal localsubject = session.getLocalPrincipal();
            System.err.println("local subject: " + localsubject);
            Principal peersubject = session.getPeerPrincipal();
            System.err.println("peer subject: " + peersubject);

            OutputStream os = sslSocket.getOutputStream();
            System.out.println("peer host: " + session.getPeerHost() + ", port: " + session.getPeerPort());
            RunTerminal runTerminal = new RunTerminal(sslSocket);
            runTerminal.start();
            while ((line = runTerminal.readLine()) != null && !line.isEmpty()) {
                line += "\n";
                os.write(line.getBytes());
            }
            sslSocket.close();
            System.out.println("Session closed");
        } while (line == null ? true : !line.isEmpty());
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 1) {
            System.err.println("Usage: ChatServer <port>");
        }
        int port = Integer.parseInt(args[0]);
        new ChatServer().chat(port);
    }
}
