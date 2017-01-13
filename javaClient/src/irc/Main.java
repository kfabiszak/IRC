package irc;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.collections.ObservableList;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.*;
import java.net.Socket;
import java.net.UnknownHostException;


public class Main extends Application {



    private static Stage primaryStage;
    private static Parent root;
    private static String server = null;
    private static int port = 0;
    private static String login = null;

    private static Socket socket;
    private static OutputStreamWriter outputStreamWriter;
    private static DataOutputStream outToServer;
    private static DataInputStream inFromServer;
    private static BufferedWriter bwriter;


    static void sendString(BufferedWriter bw, String str) {
        try {
            bw.write(str + "\r");
            bw.flush();
        }
        catch (Exception e) {
            System.out.println("Exception: "+e);
        }
    }

    public static String getServer() {
        return server;
    }

    public static int getPort() {
        return port;
    }

    public static String getLogin() {
        return login;
    }

    public static void showChat() throws IOException{
        if(server!=null){
            if(port!=0){
                if(login.trim().length() > 0){
                    connect();
                }else{
                    System.out.println("Login empty");
                }
            }else{
                System.out.println("Port empty");
            }
        }else{
            System.out.println("Server empty");
        }
        FXMLLoader loader = new FXMLLoader();
        loader.setLocation(Main.class.getResource("IRC.fxml"));
        root = loader.load();
        primaryStage.setScene(new Scene(root));
        primaryStage.show();
    }

    public static void setServer(String text){
        server = text;
    }

    public static void setLogin(String name){
        login = name;
    }

    public static void setPort(int number){
        port = number;
    }

    public static void showLoginView() throws IOException{
        FXMLLoader loader = new FXMLLoader();
        loader.setLocation(Main.class.getResource("login.fxml"));
        root = loader.load();
        primaryStage.setScene(new Scene(root));
        primaryStage.show();
    }

    public static void connect(){
        try {
            socket = new Socket(server, port);
            System.out.println("*** Connected to server.");
            outputStreamWriter = new OutputStreamWriter(socket.getOutputStream());
            System.out.println("*** Opened OutputStreamWriter.");
            bwriter = new BufferedWriter(outputStreamWriter);
            System.out.println("*** Opened BufferedWriter.");
            outToServer = new DataOutputStream(socket.getOutputStream());
            System.out.println("*** Opened DataOutputStream.");
            inFromServer = new DataInputStream(socket.getInputStream());
            System.out.println("*** Opened DataInputStream.");
        } catch (UnknownHostException e) {
            System.err.println("Don't know about host: hostname");
        } catch (IOException e) {
            System.err.println("Couldn't get I/O for the connection to: hostname");
        }
    }

    public static void disconnect(){
        try{
            bwriter.close();
            System.out.println("*** Closed BufferedWriter.");
            outputStreamWriter.close();
            System.out.println("*** Closed OutputStreamWriter.");
            outToServer.close();
            System.out.println("*** Closed DataOutputStream.");
            inFromServer.close();
            System.out.println("*** Closed DataInputStream.");
            socket.close();
            System.out.println("*** Disconnected from server.");
        }catch (Exception e){
            System.out.println(e);
        }
    }


    @Override
    public void start(Stage primaryStage) throws Exception{
        this.primaryStage = primaryStage;
        this.primaryStage.setTitle("IRC");

        showLoginView();


//        server   = "192.168.43.20";
//        port = 3012;


//        Socket socket = new Socket(getServer(), getPort());
//        System.out.println("*** Connected to server.");
//        OutputStreamWriter outputStreamWriter = new OutputStreamWriter(socket.getOutputStream());
//        System.out.println("*** Opened OutputStreamWriter.");
//        bwriter = new BufferedWriter(outputStreamWriter);
//        System.out.println("*** Opened BufferedWriter.");
//        outToServer = new DataOutputStream(socket.getOutputStream());
//        System.out.println("*** Opened DataOutputStream.");

//        Sender sender = new Sender();
//        Platform.runLater(new Sender());
//        sender.run();
    }

    public static void send(String text) throws IOException{
        int len = text.length() + 1;
//        outToServer.write(len);
//        sendString(bwriter, text);
        System.out.println(len);
        System.out.println(text);
    }

    public static void login(String text) throws IOException{
        send("#login#" + login);
    }


    public static void main(String[] args) {
        launch(args);

        disconnect();
        System.out.println(getPort());
        System.out.println(getServer());
        System.out.println(getLogin());
    }
}
