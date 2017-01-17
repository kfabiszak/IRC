package irc;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Ja on 2017-01-15.
 */
public class Room {

    private int number;
    private String[] users = null;
    private List<Message> messages = new ArrayList<Message>();
    private  boolean joined = false;


    public Room(int nr){
        number = nr;
    }

    public int getNumber() {
        return number;
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
}
