package irc;

import irc.Main;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.stage.Stage;
import javafx.scene.control.TextField;

import java.io.IOException;

/**
 * Created by Ja on 2017-01-11.
 */
public class LoginController {

    private Main main;

    public TextField login;
    public TextField address;
    public TextField port;
    public Button button;

    @FXML
    private void login() throws IOException{
        main.setServer(address.getText());
        main.setLogin(login.getText());
        main.setPort(Integer.parseInt(port.getText()));
        main.showChat();
    }

    @FXML
    public void enter(KeyEvent e) throws IOException{
        if(e.getCode().equals(KeyCode.ENTER)) {
            login();
        }
    }

}
