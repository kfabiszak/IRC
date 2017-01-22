package irc;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Ja on 2017-01-15.
 */
public class Room {

    //Numer pokoju
    private int number;
    //Lista użytkowników w pokoju
    private String[] users = null;
    //Lista wiadomości w pokoju
    private List<Message> messages = new ArrayList<Message>();
    //Informacja czy użytkownik dołączył do tego pokoju
    private boolean joined = false;
    //Informacja czy w pokoju jest nowa wiadomość
    private boolean newMess = false;


    public Room(int nr){
        setNumber(nr);
    }

    public boolean isJoined() {
        return joined;
    }

    public void setJoined(boolean joined) {
        this.joined = joined;
    }

    public String[] getUsers() {
        return users;
    }

    public void setUsers(String[] users) {
        this.users = users;
    }

    public void addMessage(String text, String name) {
        getMessages().add(new Message(name, text));
    }

    public List<Message> getMessages() {
        return messages;
    }

    public void setMessages(List<Message> messages) {
        this.messages = messages;
    }

    public void setNumber(int number) {
        this.number = number;
    }

    public boolean isNewMess() {
        return newMess;
    }

    public void setNewMess(boolean newMess) {
        this.newMess = newMess;
    }
}
