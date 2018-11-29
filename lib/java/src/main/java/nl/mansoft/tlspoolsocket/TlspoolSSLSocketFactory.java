/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package nl.mansoft.tlspoolsocket;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import javax.net.ssl.SSLSocketFactory;

/**
 *
 * @author hfman
 */
public class TlspoolSSLSocketFactory extends SSLSocketFactory {

    @Override
    public String[] getDefaultCipherSuites() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public String[] getSupportedCipherSuites() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Socket createSocket(Socket socket, String host, int port, boolean autoClose) throws IOException {
        TlspoolSocket tlspoolSocket = new TlspoolSocket(socket);
        tlspoolSocket.startTls();
        return tlspoolSocket;
    }

    @Override
    public Socket createSocket(String string, int i) throws IOException, UnknownHostException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Socket createSocket(String string, int i, InetAddress ia, int i1) throws IOException, UnknownHostException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Socket createSocket(InetAddress ia, int i) throws IOException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Socket createSocket(InetAddress ia, int i, InetAddress ia1, int i1) throws IOException {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    public static void main(String[] args) throws Exception {
        String host = "localhost";
        int port = 12345;
        Socket socket = new Socket(host, port);
        SSLSocketFactory factory = new TlspoolSSLSocketFactory();
        Socket sslSocket = factory.createSocket(socket, host, port, true);
        sslSocket.getOutputStream().write(new byte[] { 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x0a  });
        InputStream is = sslSocket.getInputStream();
        byte[] barr = new byte[10];
        is.read(barr, 0, barr.length);
        System.out.println(new String(barr));
        //sslSocket.joinReadEncryptedThread();
        //sslSocket.joinWriteEncryptedThread();
        System.out.println("EXITING");
    }
}
