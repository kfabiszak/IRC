package irc;

import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.TextArea;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.control.ListView;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.scene.text.Text;
import javafx.scene.text.TextFlow;

import java.io.IOException;
import java.util.Arrays;

public class Controller {

    private Process p;
    private boolean notInRoom = false;

    //Pole do wpisywania wiadomości
    @FXML
    private TextArea typing;
    //Pole do wyświetlania chatu
    @FXML
    private TextFlow chat;
    //Przewijanie chatu
    @FXML
    private ScrollPane scroll;
    //Lista pokoi
    @FXML
    private ListView<String> roomList;
    //Lista użytkowników
    @FXML
    private ListView<String> userList;

    //Kończenie procesu głównego okna
    public void destroy() {
        if (p != null) {
            p.destroy();
            System.out.println("Main controller destroyed.");
        }
    }

    //Wylogowanie z aktualnego chatu i wyświetlenie okna logowania
    @FXML
    private void relogin() throws IOException{
        Main.disconnect();
        destroy();
        Main.setLogged(false);
        Main.showLoginView();
    }

    //Funkcja wywoływana na enter w polu do wpisywania wiadomości - wysłanie wiadomości
    @FXML
    public void enter(KeyEvent e){
        if(e.getCode().equals(KeyCode.ENTER)){
            Platform.runLater(new Runnable() {
                public void run() {
                    if (typing.getText().trim().length() > 0){
                        try {
                            Main.sendMess(typing.getText());
                            Main.send("MessageSent");
                        } catch (Exception e){
                            System.out.println("Send message on enter error: " + e);
                        }
                        typing.clear();
                        scroll.setVvalue(1.0);
                    } else {
                        typing.clear();
                    }
                }
            });
        }
    }

    //Funkcja wywoływana na przycisk "Relogin" - wylogowanie
    @FXML
    public void enterButton(KeyEvent e) throws IOException{
        if(e.getCode().equals(KeyCode.ENTER))
            relogin();
    }

    //Odczyt wiadomości z serwera w nowym wątku
    public void readChat() {
        try {
            p = new ProcessBuilder("ping", "stackoverflow.com", "-n", "100").start();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    int header;
                    try {
                        while((header = Main.readHeader()) > 0) {
                            String line = "";
                            try {
                                while(line.length() < header) {
                                    line += Main.read(header);
                                    System.out.println("Received(" + header + "): " + line);
                                }
                                decode(line);
                            } catch (Exception e) {
                                System.out.println("Read header + decode error: " + e);
                            }
                        }
                    } catch (Exception e) {
                        System.out.println("while loop in readChat error: " + e);
                    }
                }
            }).start();
        } catch (IOException e) {
            System.out.println("thread in readChat error: " + e);
        }
    }

    //Dekodowanie wiadomości (podział na komendę i argumenty
    public void decode(String text){
        String[] words = text.split("#");
        String cmd = "", arg = "", nick ="", msg = "";
        for (int i = 1; i < words.length; i++) {
//            System.out.println(i + ": " + words[i]);
            switch (i){
                case 1:
                    cmd = words[i];
                    break;
                case 2:
                    arg = words[i];
                    break;
                case 3:
                    nick = words[i];
                    break;
                case 4:
                    msg = words[i];
                    break;
            }
        }
        if(cmd.equals("users")){
            String[] oldList = Main.getRooms()[Integer.parseInt(arg)].getUsers();
            String[] usersList = new String[words.length-4];
            for (int i = 3; i < words.length-1; i++){
                if (words[i].equals(" ")) {
                    break;
                }
                if (!words[i].trim().equals("system")) {
                    if (!words[i].equals(Main.getLogin())) {
                        usersList[i - 3] = words[i].trim();
                    } else {
                        usersList[i - 3] = words[i].trim() + "(me)";
                    }
                } else {
                    usersList[i - 3] = " ";
                }
            }
            if(!Main.isLogged()) {
                if(!Arrays.asList(usersList).contains(Main.getLogin() + "(me)")) {
                    try {
                        Main.login();
                        System.out.println("Login with check unique.");
                    } catch (Exception e) {
                        System.out.println("Login after check if taken error: " + e);
                    }
                } else {
                    System.out.println("Login already taken.");
                    Main.disconnect();
                    destroy();
                    Main.raiseLoginTaken();
                }
            }
            if (oldList != null) {
//                for (int i = 0; i < oldList.length; i++) { //TODO foreach sprawdzic czy dziala
//                    if (!Arrays.asList(usersList).contains(oldList[i])) {
//                        if (!oldList[i].equals(" "))
//                            if (!oldList[i].equals(Main.getLogin() + "(me)")) {
//                                Main.getRooms()[Integer.parseInt(arg)].addMessage("User " + oldList[i] + " left the room.", "system");
//                            }
//                    }
//                }
                for (String u : oldList) {
                    if (!Arrays.asList(usersList).contains(u)) {
                        if (!u.equals(" "))
                            if (!u.equals(Main.getLogin() + "(me)")) {
                                Main.getRooms()[Integer.parseInt(arg)].addMessage("User " + u + " left the room.", "system");
                            }
                    }
                }
//                for (int i = 0; i < usersList.length; i++) {
//                    if (!Arrays.asList(oldList).contains(usersList[i])) {
//                        if (!usersList[i].equals(" "))
//                            if (!usersList[i].equals(Main.getLogin() + "(me)")) {
//                                Main.getRooms()[Integer.parseInt(arg)].addMessage("User " + usersList[i] + " joined the room.", "system");
//                            } else {
//                                Main.getRooms()[Integer.parseInt(arg)].addMessage("User " + Main.getLogin() + " joined the room.", "system");
//                            }
//                    }
//                }
                for (String u : usersList) {
                    if (!Arrays.asList(oldList).contains(u)) {
                        if (!u.equals(" "))
                            if (!u.equals(Main.getLogin() + "(me)")) {
                                Main.getRooms()[Integer.parseInt(arg)].addMessage("User " + u + " joined the room.", "system");
                            } else {
                                Main.getRooms()[Integer.parseInt(arg)].addMessage("User " + Main.getLogin() + " joined the room.", "system");
                            }
                    }
                }
            }
            Main.getRooms()[Integer.parseInt(arg)].setUsers(usersList);
            setRoom();
        } else if (cmd.equals("send")) {
            Main.getRooms()[Integer.parseInt(arg)].addMessage(msg, nick);
            show(Integer.parseInt(arg));
        } else if (cmd.equals("success")) {
            if(arg.equals("connect")) {
                Main.setConnected(true);
                System.out.println("Success connected.");
            } else if (arg.equals("login")) {
                Main.setLogged(true);
                System.out.println("Success logged.");
                Platform.runLater(new Runnable() {
                    public void run() {
                        try {
                            Main.showChat();
                        } catch (Exception e) {
                            System.out.println("Show chat after success login error: " + e);
                        }
                    }
                });
            } else if (arg.equals("join")) {
                try {
                    Main.joinRoom(Integer.parseInt(nick));
                    Main.getRooms()[Integer.parseInt(nick)].addMessage("User " + Main.getLogin() + " joined the room.", "system");
                } catch (Exception e) {
                    System.out.println("Join room (with success from server) error: " + e);
                }
            } else if (arg.equals("leave")) {
                try{
                    Main.leaveRoom(Integer.parseInt(nick));
                    Main.getRooms()[Integer.parseInt(nick)].setUsers(null);
                    setRoom();
                } catch (Exception e) {
                    System.out.println("Leave room (with success from server) error: " + e);
                }
            }
        } else if (cmd.equals("error")) {
//            if(userList.getSelectionModel().getSelectedIndex() == Integer.parseInt(arg)) {
                if (arg.equals("join") && Main.getRoom() != 0) {
                    showError("Room " + nick + " is full.");
                }
//            }
        }
    }

    //Wyświetlanie wiadomości w oknie chatu
    @FXML
    public void show(final int roomNum) {
        if (roomNum == Main.getRoom()) {
            Platform.runLater(new Runnable() {
                public void run() {
                    chat.getChildren().clear();
                    if (notInRoom) {
                        showError("You didn't join room " + roomNum + " yet.");
                        notInRoom = false;
                    }
                    for (Message object: Main.getRooms()[roomNum].getMessages()) {
                        Text name = new Text (object.getNick() + ": ");
                        name.setFont(Font.font("Verdana", FontWeight.BOLD, 13));
                        if (!object.getNick().equals(Main.getLogin())) {
                            if (object.getNick().equals("system")) {
                                name.setFill(Color.RED);
                            } else {
                                name.setFill(Color.BLUE);
                            }
                        } else {
                            name.setFill(Color.GREEN);
                        }
                        chat.getChildren().add(name);
                        Text t = new Text(object.getMessage() + "\n"); //TODO trimujemy wiadomosci czy nie ?1! //pogrubiac wiadomosci od systemu
                        chat.getChildren().add(t);
                    }
                    scroll.setVvalue(1.0);
                }
            });
        }
    }

    //Wyświetlanie komunikatu o błędzie w oknie chatu
    @FXML
    public void showError(String text) {
        Text t = new Text(text + "\n");
        t.setFont(Font.font("Verdana", FontWeight.BOLD, 13));
        t.setFill(Color.RED);
        final Text error = t;
        Platform.runLater(new Runnable() {
            public void run() {
                chat.getChildren().add(error);
            }
        });
    }

    //Zmiana pokoju po wybraniu z listy pokoi
    @FXML
    public void setRoom(){
        Main.setRoom(roomList.getSelectionModel().getSelectedIndex());
        final int roomNum = roomList.getSelectionModel().getSelectedIndex();
        Platform.runLater(new Runnable() {
            public void run() {
                userList.setItems(null);
                show(roomNum);
            }
        });
        if(Main.getRooms()[roomNum].getUsers() != null) {
            final ObservableList<String> users = FXCollections.observableArrayList(Main.getRooms()[roomNum].getUsers());
//            final int me = users.indexOf(Main.getLogin());
            Platform.runLater(new Runnable() {
                public void run() {
                    userList.setItems(null);
                    userList.setItems(users);
//                    userList.setStyle("-fx-control-inner-background: blue;"); //TODO pogrubic kim jestem sie nie da dopisze (me) za moim loginem i tyle
                    System.out.println("UserList of room " + roomNum + " set.");
                }
            });
        } else {
            System.out.println("No access to room " + roomNum + " without join.");
            notInRoom = true;
        }
    }

    //Obsługa przycisku "Join" - dołączenie do wybranego pokoju
    @FXML
    public void joinRoom() {
        final int roomNum = roomList.getSelectionModel().getSelectedIndex();
        try {
            if (!Main.getRooms()[roomNum].isJoined()) {
                Main.send("#join#" + roomNum);
                Platform.runLater(new Runnable() {
                    public void run() {
                        roomList.requestFocus();
                    }
                });
            } else {
                showError("You're already in room " + roomNum + ".");
            }
        } catch (Exception e) {
            System.out.println("Join room button error: " + e);
        }
    }

    //Obsługa przycisku "Leave" - opuszczenie wybranego pokoju
    @FXML
    public void leaveRoom() {
        final int roomNum = roomList.getSelectionModel().getSelectedIndex();
        try {
            Main.send("#leave#" + roomNum);
            Platform.runLater(new Runnable() {
                public void run() {
                    roomList.requestFocus();
                    if(!Main.getRooms()[roomNum].isJoined())
                        showError("You are not in room " + roomNum + ".");
                    if (roomNum == 0) {
                        showError("You can't leave room 0.");
                    }
                }
            });
        } catch (Exception e) {
            System.out.println("Leave room button error: " + e);
        }
    }

    //Inicjalizacja głównego okna aplikacji
    @FXML
    public void initialize() {
        if (!Main.isConnected())
            Main.raiseCantConnect();
        System.out.println("Initializing mainController.");
        ObservableList<String> rooms = FXCollections.observableArrayList(Main.getRoomList());
        roomList.setItems(rooms);
        roomList.getSelectionModel().selectFirst();
        typing.requestFocus();
//        try{
//            main.send("StartMessage");
//        } catch (Exception e) {
//            System.out.println(e);
//        }
        readChat();
    }

}
