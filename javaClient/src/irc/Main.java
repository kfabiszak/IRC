package irc;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.*;
import java.net.Socket;
import java.net.UnknownHostException;

import java.text.Normalizer;
import java.util.ArrayList;
import java.util.regex.Pattern;

public class Main extends Application {


    //Server's IP address
    private static String server = null;
    //Port number
    private static int port = 0;
    //User's login
    private static String login = null;
    //Room shown in the main chat
    private static int room = 0;
    //List of rooms
    private static String[] roomList = new String[20];
    //Room structures
    private static Room[] rooms = new Room[20];

    private static Socket socket;
    private static OutputStreamWriter outputStreamWriter;
    private static BufferedWriter bwriter;
    private static DataOutputStream outToServer = null;
    private static DataInputStream inFromServer = null;
    private static InputStreamReader inputStreamReader = null;
    private static BufferedReader breader;

    private static Stage primaryStage;
    private static Parent rootChat;
    //Controller of the main window
    private static Controller controller = null;
    //Controller of the login window
    private static LoginController loginController = null;

    //Status of connection with server
    private static boolean connected = false;
    //Status of logging to server
    private static boolean logged = false;


    public static String getLogin() {
        return login;
    }

    public static int getRoom() {
        return room;
    }

    public static Room[] getRooms() {
        return rooms;
    }

    public static String[] getRoomList() {
        return roomList;
    }

    public static void setRoom(int room) {
        Main.room = room;
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

    public static boolean isConnected() {
        return connected;
    }

    public static void setConnected(boolean connected) {
        Main.connected = connected;
    }

    public static boolean isLogged() {
        return logged;
    }

    public static void setLogged(boolean logged) {
        Main.logged = logged;
    }

    //Usuwanie znaków specjalnych
    public static String deAccent(String str) {
        String nfdNormalizedString = Normalizer.normalize(str, Normalizer.Form.NFD);
        Pattern pattern = Pattern.compile("\\p{InCombiningDiacriticalMarks}+");
        return pattern.matcher(nfdNormalizedString).replaceAll("").replaceAll("ł","l").replaceAll("Ł","L");
    }

    //Dodawanie znaku dołączenia do pokoju (!)
    public static void addJoin(int roomNum) {
        roomList[roomNum] += " (!)";
        controller.refreshRooms(roomNum);
    }

    //Dodawanie znaku nowej wiadomości w pokoju (*)
    public static void addNew(int roomNum) {
        if(!rooms[roomNum].isNewMess()) {
            roomList[roomNum] += " (*)";
            rooms[roomNum].setNewMess(true);
            controller.refreshRooms(roomNum);
        }
    }

    //Usuwanie znaków przy numerze pokoju, zostawiam (!) jeśli jest w tym pokoju
    public static void clearRoom(int roomNum) {
        if(roomNum >= 0 && roomNum < 20) {
            rooms[roomNum].setNewMess(false);
            roomList[roomNum] = Integer.toString(roomNum);
            if (rooms[roomNum].isJoined())
                addJoin(roomNum);
            controller.refreshRooms(roomNum);
        }
    }

    //Wysyłanie tekstu przez Buffered Writer
    public static void sendString(BufferedWriter bw, String str) {
        try {
            bw.write(str);
            bw.flush();
        } catch (Exception e) {
            System.out.println("sendString through buffered writer error: " + e);
        }
    }

    //Wysyłanie headera + tekstu
    public static void send(String text) throws IOException{
        text = deAccent(text.trim());
        int len = text.length();
        outToServer.write(len);
        sendString(bwriter, text);  //trim - usuwam białe znaki na końcu
        System.out.println("Sent(" + len + "): " + text);
    }

    //Wysyłanie zakodowanej wiadomości
    public static void sendMess(String text) throws IOException{
        if(rooms[room].isJoined()) {
            send("#send#" + getRoom() + "#" + text);
        } else {
            controller.showError("You didn't join this room.");
        }
    }

    //Odczyt wiadomości
    public static String read(int header) throws IOException{
        byte[] bytes = new byte[header];
        inFromServer.readFully(bytes);
        return new String(bytes);
    }

    //Odczyt headera
    public static int readHeader() throws IOException{
        if (inFromServer != null) {
            return inFromServer.readInt();
        } else {
            return 0;
        }
    }

    //Logowanie
    public static void login() throws IOException{
        send("#login#" + login);
    }

    //Inicjalizowanie pokoi
    public static void createRooms() {
        for (int i = 0; i < getRooms().length; i++){
            getRooms()[i] = new Room(i);
            getRoomList()[i] = Integer.toString(i);
        }
    }

    //Opuszczanie pokoju
    public static void leaveRoom(int number) throws IOException{
        if(number != 0){
            if (getRooms()[number].isJoined()) {
                getRooms()[number].setJoined(false);
                getRooms()[number].setMessages(new ArrayList<Message>());
            } else {
                System.out.println("Not in this room.");
                controller.showError("You are not in room " + number);
            }
        } else {
            System.out.println("Can't leave room 0.");
            controller.showError("You can't leave room 0");
        }
    }

    //Dołączanie do pokoju
    public static void joinRoom(int number) throws IOException{
        if (!getRooms()[number].isJoined()){
            getRooms()[number].setJoined(true);
            System.out.println("Joined room: " + number);
        } else {
            //TODO err juz jestes w tym pokoju
            System.out.println("Already in this room.");
        }
    }

    //Inicjalizacja klienta (łączenie + ładowanie głównego okna aplikacji)
    public static void initialize() throws IOException {
        if (server != null) {
            if (port != 0) {
                if (!connected){
                    connect();
                }
                FXMLLoader loader = new FXMLLoader();
                loader.setLocation(Main.class.getResource("IRC.fxml"));
                rootChat = loader.load();
                controller = loader.getController();
            } else {
                System.out.println("Port empty.");
            }
        } else {
            System.out.println("Server empty.");
        }
    }

    //Wyświetlanie głównego okna aplikacji
    public static void showChat() throws IOException {
        if (isConnected()) {
            if (isLogged()) {
                primaryStage.setScene(new Scene(rootChat));
                primaryStage.setTitle("IRC - " + login);
                primaryStage.show();
            } else {
                System.out.println("Can't show chat - not logged in.");
            }
        } else {
            loginController.connectError.setVisible(true);
            System.out.println("Can't show chat - not connected.");
        }
    }

    //Ładowanie i wyświetlanie okna logowania
    public static void showLoginView() throws IOException{
        FXMLLoader loader = new FXMLLoader();
        loader.setLocation(Main.class.getResource("login.fxml"));
        Parent rootLogin = loader.load();
        loginController = loader.getController();
        primaryStage.setScene(new Scene(rootLogin));
        primaryStage.setTitle("IRC - Log In");
        primaryStage.show();
    }

    //Łączenie z serwerem
    public static void connect(){
        try {
            socket = new Socket(server, port);
            System.out.println("*** Connected to server.");
            System.out.println("Socket: " + socket);
            outputStreamWriter = new OutputStreamWriter(socket.getOutputStream());
            System.out.println("*** Opened OutputStreamWriter.");
            bwriter = new BufferedWriter(outputStreamWriter);
            System.out.println("*** Opened BufferedWriter.");
            outToServer = new DataOutputStream(socket.getOutputStream());
            System.out.println("*** Opened DataOutputStream.");
            inFromServer = new DataInputStream(socket.getInputStream());
            System.out.println("*** Opened DataInputStream.");
            inputStreamReader = new InputStreamReader(socket.getInputStream());
            System.out.println("*** Opened InputStreamReader.");
            breader = new BufferedReader(inputStreamReader);
            System.out.println("*** Opened BufferedReader.");
        } catch (UnknownHostException e) {
            System.out.println("Don't know about host: hostname error: " + e);
        } catch (IOException e) {
            System.out.println("Couldn't get I/O for the connection to: hostname error: " + e);
        }
    }

    //Rozłączanie z serwerem
    public static void disconnect(){
        if(!socket.isClosed()) {
            try {
                send("#logout");
                bwriter.close();
                System.out.println("*** Closed BufferedWriter.");
                outputStreamWriter.close();
                System.out.println("*** Closed OutputStreamWriter.");
                outToServer.close();
                System.out.println("*** Closed DataOutputStream.");
                inFromServer.close();
                System.out.println("*** Closed DataInputStream.");
                breader.close();
                System.out.println("*** Closed BufferedReader.");
                inputStreamReader.close();
                System.out.println("*** Closed InputStreamReader.");
                socket.close();
                System.out.println("*** Disconnected from server.");
                connected = false;
            } catch (Exception e) {
                System.out.println("Disconnect error: " + e);
            }
        }
    }

    //Wyświetlanie błędu - login zajęty
    public static void raiseLoginTaken() {
        loginController.loginErrorTaken.setVisible(true);
    }

    //Wyświetlanie błędu połączenia z serwerem
    public static void raiseCantConnect(){
        loginController.connectError.setVisible(true);
    }

    @Override
    public void start(Stage primStage) throws Exception{
        createRooms();
        getRooms()[0].setJoined(true);
        primaryStage = primStage;

        showLoginView();
    }

    @Override
    public void stop() throws Exception{
        super.stop();
        disconnect();
        controller.destroy();
    }

    public static void main(String[] args) {
        launch(args);

        controller.destroy();
        disconnect();
    }

}
