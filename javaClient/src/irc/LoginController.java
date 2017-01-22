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

    //Pole do pobierania loginu
    @FXML
    public TextField login;
    //Pole do pobierania adresu IP serwera
    @FXML
    public TextField address;
    //Pole do pobierania numeru portu
    @FXML
    public TextField port;
    //Przycisk do logowania
    @FXML
    public Button button;
    //Komunikat o błędzie - zbyt długi login
    @FXML
    public Label loginErrorLong;
    //Komunikat o błędzie - pusty login
    @FXML
    public Label loginErrorEmpty;
    //Komunikat o błędzie - zajęty login
    @FXML
    public Label loginErrorTaken;
    //Komunikat o błędzie - zarezerwowany login
    @FXML
    public Label loginErrorSystem;
    //Komunikat o błędzie łączenia z serwerem
    @FXML
    public Label connectError;

    //Obsługa przycisku "Login" (łączenie i logowanie do serwera)
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

    //Obsługa klawisza enter przy logowaniu
    @FXML
    public void enter(KeyEvent e) throws IOException{
        if(e.getCode().equals(KeyCode.ENTER)) {
            login();
        }
    }

    //Chowanie komunikatów o błędach na "focus" na polu
    public void focus(){
        loginErrorEmpty.setVisible(false);
        loginErrorLong.setVisible(false);
        loginErrorTaken.setVisible(false);
        loginErrorSystem.setVisible(false);
        connectError.setVisible(false);
    }

    //Inicjalizacja okna do logowania - obsługa "focusu" na polach do wpisywania danych
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
