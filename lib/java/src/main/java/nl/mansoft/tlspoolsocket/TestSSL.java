package nl.mansoft.tlspoolsocket;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.security.cert.Certificate;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.net.ssl.HttpsURLConnection;

public class TestSSL {
    public static void testSSL(String urlString, boolean useTlspool) {
        try {
            System.err.println("Visiting: " + urlString);
            URL url = new URL(urlString);
            URLConnection urlConnection = url.openConnection();
            HttpsURLConnection httpsURLConnection = null;
            if (urlConnection instanceof HttpsURLConnection) {
                httpsURLConnection = (HttpsURLConnection) urlConnection;
                if (useTlspool) {
                    httpsURLConnection.setSSLSocketFactory(new TlspoolSSLSocketFactory());
                }
            }
            urlConnection.connect();
//            if (httpsURLConnection != null) {
//                System.out.println(httpsURLConnection.getCipherSuite());
//                Certificate certificates[] = httpsURLConnection.getLocalCertificates();
//                if (certificates != null) {
//                    for (Certificate certificate : certificates) {
//                        System.out.println(certificate);
//
//                    }
//                }
//            }
//          BufferedReader in = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));
            Map<String, List<String>> map = urlConnection.getHeaderFields();
            for (Map.Entry<String, List<String>> e:  map.entrySet()) {
                System.err.println(e.getKey());
                for (String value : e.getValue()) {
                    System.err.println("\t" + value);
                }
            }
            byte[] buf = new byte[1024*1024];
            int bytesread;
            int offset = 0;
            while ((bytesread = urlConnection.getInputStream().read(buf, offset, buf.length - offset)) > 0) {
                System.err.println("read: " + bytesread);
                offset += bytesread;
            }
            System.err.println("total size: " + offset);
            System.out.write(buf, 0, offset);
//            while ((inputLine = in.readLine()) != null)
//                System.out.println(inputLine);
//            in.close();
        } catch (IOException ex) {
            Logger.getLogger(TestSSL.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

}


