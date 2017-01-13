package irc;

import irc.Main;
import javafx.application.Platform;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.TextArea;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.control.ListView;
import javafx.scene.text.Text;
import javafx.scene.text.TextFlow;

import java.io.IOException;

public class Controller {

    private Main main;

    @FXML
    private Button relogin;

    @FXML
    private void relogin() throws IOException{
        main.showLoginView();
    }

    @FXML
    private TextArea typing;

    @FXML
    private TextFlow chat;

    @FXML
    private ScrollPane scroll;

    @FXML
    private ListView roomList;

    @FXML
    private ListView userList;

    @FXML
    public void enter(KeyEvent e){
        if(e.getCode().equals(KeyCode.ENTER)){
            Platform.runLater(new Runnable() {
                public void run() {
                    if (typing.getText().trim().length() > 0){
//                        main.setMessagge(typing.getText()); //nie potrzeba juz message w maine
                        Text t = new Text(typing.getText());
                        chat.getChildren().add(t);
                        try {
                            main.send(typing.getText());
                        } catch (Exception e){
                            System.out.println(e);
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

    @FXML
    public void enterButton(KeyEvent e) throws IOException{
        if(e.getCode().equals(KeyCode.ENTER))
            relogin();
    }



    @FXML
    public void refresh(){
        Platform.runLater(new Runnable() {
            public void run() {
                while(true){
                    System.out.println("jestem");
                }
            }
        });
    }

    public void changeRoom(){
        Platform.runLater(new Runnable() {
            public void run() {
                while(true){
                    System.out.println("jestem");
                }
            }
        });
    }

}
