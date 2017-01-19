package irc;

/**
 * Created by Ja on 2017-01-15.
 */
public class Message {

    //Nick użytkownika, który wysłał wiadomość
    private String nick;
    //Treść wiadomości
    private String message;

    Message(String name, String text) {
        nick = name;
        message = text;
    }

    public String getMessage() {
        return message;
    }

    public String getNick() {
        return nick;
    }

}
