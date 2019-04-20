package nl.mansoft.tlspoolsocket;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.net.URL;
import java.net.HttpURLConnection;
import java.security.cert.Certificate;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.net.ssl.HttpsURLConnection;

public class TestSSL {
    public static void direct(InputStream is) throws IOException {
        byte[] buf = new byte[1024*1024];
        int bytesread;
        int offset = 0;
        while ((bytesread = is.read(buf, offset, buf.length - offset)) > 0) {
            System.err.println("read: " + bytesread);
            offset += bytesread;
        }
        System.err.println("total size: " + offset);
        System.out.write(buf, 0, offset);
    }

    public static void printCertificates(HttpsURLConnection httpsURLConnection) {
        if (httpsURLConnection != null) {
            System.out.println(httpsURLConnection.getCipherSuite());
            Certificate certificates[] = httpsURLConnection.getLocalCertificates();
            if (certificates != null) {
                for (Certificate certificate : certificates) {
                    System.out.println(certificate);
                }
            }
        }
    }

    public static void printResponseHeaders(Map<String, List<String>> headerFields) {
        for (Map.Entry<String, List<String>> e: headerFields.entrySet()) {
            System.err.println(e.getKey());
            for (String value : e.getValue()) {
                System.err.println("\t" + value);
            }
        }
    }

    public static void testSSL(String urlString, boolean useTlspool) {
        try {
            System.err.println("Visiting: " + urlString);
            URL url = new URL(urlString);
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            if (urlConnection instanceof HttpsURLConnection) {
                HttpsURLConnection httpsURLConnection = (HttpsURLConnection) urlConnection;
                if (useTlspool) {
                    httpsURLConnection.setSSLSocketFactory(new TlspoolSSLSocketFactory());
                }
                //printCertificates(httpsURLConnection);
            }
            urlConnection.connect();
            printResponseHeaders(urlConnection.getHeaderFields());
            direct(urlConnection.getInputStream());
            urlConnection.disconnect();
        } catch (IOException ex) {
            Logger.getLogger(TestSSL.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}


