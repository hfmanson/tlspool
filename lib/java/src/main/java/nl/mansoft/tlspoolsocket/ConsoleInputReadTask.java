package nl.mansoft.tlspoolsocket;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.concurrent.Callable;

public class ConsoleInputReadTask implements Callable<String> {
    private boolean cont = true;

    public void stop() {
        cont = false;
    }

    public String call() throws IOException {
        BufferedReader br = new BufferedReader(
                new InputStreamReader(System.in));
        System.out.println("ConsoleInputReadTask run() called.");
        String input;
        try {
            // wait until we have data to complete a readLine()
            while (!br.ready()) {
                Thread.sleep(200);
                if (!cont) {
                    System.out.println("ConsoleInputReadTask() stopped");
                    return null;
                }
            }
            input = br.readLine();
        } catch (InterruptedException e) {
            System.out.println("ConsoleInputReadTask() cancelled");
            return null;
        }
        return input;
    }
}
