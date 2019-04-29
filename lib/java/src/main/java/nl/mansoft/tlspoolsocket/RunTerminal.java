/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package nl.mansoft.tlspoolsocket;

import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author hfman
 */
public class RunTerminal {
    private Socket socket;
    private ConsoleInputReadTask consoleInputReadTask;
    private final Thread socketThread = new Thread() {
        @Override
        public void run() {
            InputStream is = null;
            try {
                byte[] buf = new byte[4096];
                is = socket.getInputStream();
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
                Logger.getLogger(ChatClient.class.getName()).log(Level.SEVERE, null, ex);
            } finally {
                try {
                    is.close();
                } catch (IOException ex) {
                    Logger.getLogger(ChatClient.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    };

    public RunTerminal(Socket socket) {
        this.socket = socket;
        consoleInputReadTask = new ConsoleInputReadTask();
    }

    public void start() {
        socketThread.start();
    }

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
}
