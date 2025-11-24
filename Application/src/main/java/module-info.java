module com.example.application {
    requires javafx.controls;
    requires javafx.fxml;
    requires java.desktop;
    requires org.eclipse.paho.client.mqttv3;


    opens com.example.application to javafx.fxml;
    exports com.example.application;
}