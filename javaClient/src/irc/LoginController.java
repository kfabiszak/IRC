package irc;

import javafx.beans.value.ChangeListener;
import javafx.beans.value.ObservableValue;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.control.TextField;

import java.io.IOException;

/**
 * Created by Ja on 2017-01-11.
 */
public class LoginController {

    @FXML
    public TextField login;
    @FXML
    public TextField address;
    @FXML
    public TextField port;
    @FXML
    public Button button;
    @FXML
    public Label loginErrorLong;
    @FXML
    public Label loginErrorEmpty;
    @FXML
    public Label loginErrorTaken;
    @FXML
    public Label loginErrorSystem;
    @FXML
    public Label connectError;

    @FXML
    private void login() throws IOException{
        Main.setServer(address.getText());
        Main.setPort(Integer.parseInt(port.getText()));
        if (login.getText().trim().length() < 15){
            if (login.getText().trim().length() > 0) {
                if (!login.getText().trim().equals("system")) {
                    Main.setLogin(login.getText().trim());
                    Main.initialize();
                } else {
                    login.clear();
                    loginErrorSystem.setVisible(true);
                    System.out.println("Login can't be 'system'.");
                }
            } else {
                login.clear();
                loginErrorEmpty.setVisible(true);
                System.out.println("Login can't be empty.");
            }
        } else {
            login.clear();
            loginErrorLong.setVisible(true);
            System.out.println("Login is too long.");
        }
    }

    @FXML
    public void enter(KeyEvent e) throws IOException{
        if(e.getCode().equals(KeyCode.ENTER)) {
            login();
        }
    }

    public void focus(){
        loginErrorEmpty.setVisible(false);
        loginErrorLong.setVisible(false);
        loginErrorTaken.setVisible(false);
        loginErrorSystem.setVisible(false);
        connectError.setVisible(false);
    }

    @FXML
    public void initialize() {
        System.out.println("Initializing loginController.");
        login.focusedProperty().addListener(new ChangeListener<Boolean> () {
            @Override
            public void changed(ObservableValue<? extends Boolean> arg0, Boolean oldPropertyValue, Boolean newPropertyValue) {
                if (newPropertyValue)
                {
                    focus();
                }
            }
        });
        port.focusedProperty().addListener(new ChangeListener<Boolean> () {
            @Override
            public void changed(ObservableValue<? extends Boolean> arg0, Boolean oldPropertyValue, Boolean newPropertyValue) {
                if (newPropertyValue)
                {
                    focus();
                }
            }
        });
        address.focusedProperty().addListener(new ChangeListener<Boolean> () {
            @Override
            public void changed(ObservableValue<? extends Boolean> arg0, Boolean oldPropertyValue, Boolean newPropertyValue) {
                if (newPropertyValue)
                {
                    focus();
                }
            }
        });
    }

}
