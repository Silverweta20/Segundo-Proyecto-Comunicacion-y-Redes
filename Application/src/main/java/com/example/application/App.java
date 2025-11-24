package com.example.application;

import org.eclipse.paho.client.mqttv3.*;
import javax.swing.*;
import java.awt.*;

public class App extends JFrame {

    // Configuración del broker
    private static final String BROKER = "tcp://broker.emqx.io:1883";
    private static final String CLIENT_ID = "DetectorGasUI-" + System.currentTimeMillis();
    private static final String TOPIC_SENSOR = "claseRedes/grupo4/cocina/MQ2";
    private static final String TOPIC_ALARMA = "claseRedes/grupo4/cocina/alarma";
    private static final String USERNAME = "emqx";
    private static final String PASSWORD = "public";
    private static final int UMBRAL = 300;

    // Componentes visuales
    private JLabel lblEstado;
    private JLabel lblNivel;
    private JButton btnConectar;
    private JButton btnAlarmaManual;
    private MqttClient client;

    public App() {
        // --- Ventana principal ---
        setTitle("Detector de Gas IoT");
        setSize(400, 300);
        setLayout(new GridLayout(4, 1));
        setDefaultCloseOperation(EXIT_ON_CLOSE);

        lblNivel = new JLabel("Nivel de gas: --", SwingConstants.CENTER);
        lblNivel.setFont(new Font("Arial", Font.BOLD, 22));

        lblEstado = new JLabel("Estado: Desconectado", SwingConstants.CENTER);
        lblEstado.setFont(new Font("Arial", Font.PLAIN, 18));
        lblEstado.setOpaque(true);
        lblEstado.setBackground(Color.LIGHT_GRAY);

        btnConectar = new JButton("Conectar al broker");
        btnConectar.addActionListener(e -> conectarMQTT());

        btnAlarmaManual = new JButton("Encender alarma manual");
        btnAlarmaManual.addActionListener(e -> enviarAlarmaManual());

        add(lblNivel);
        add(lblEstado);
        add(btnConectar);
        add(btnAlarmaManual);
    }

    private void conectarMQTT() {
        try {
            client = new MqttClient(BROKER, CLIENT_ID);
            MqttConnectOptions options = new MqttConnectOptions();
            options.setAutomaticReconnect(true);
            options.setCleanSession(true);
            options.setUserName(USERNAME);
            options.setPassword(PASSWORD.toCharArray());
            client.connect(options);

            lblEstado.setText("Estado: Conectado");
            lblEstado.setBackground(Color.GREEN);
            System.out.println("Conectado al broker: " + BROKER);

            // Suscripción
            client.subscribe(TOPIC_SENSOR, (topic, msg) -> {
                String payload = new String(msg.getPayload());
                SwingUtilities.invokeLater(() -> actualizarNivel(payload));
            });

        } catch (Exception e) {
            lblEstado.setText("Error de conexion");
            lblEstado.setBackground(Color.RED);
            e.printStackTrace();
        }
    }

    private void actualizarNivel(String valor) {
        lblNivel.setText("Nivel de gas: " + valor);

        try {
            int gas = Integer.parseInt(valor);
            if (gas > UMBRAL) {
                lblEstado.setText("PELIGRO: Nivel alto");
                lblEstado.setBackground(Color.RED);
                client.publish(TOPIC_ALARMA, new MqttMessage("ON".getBytes()));
            } else {
                lblEstado.setText("Normal");
                lblEstado.setBackground(Color.GREEN);
                client.publish(TOPIC_ALARMA, new MqttMessage("OFF".getBytes()));
            }
        } catch (NumberFormatException | MqttException e) {
            lblEstado.setText("Error de lectura");
            lblEstado.setBackground(Color.ORANGE);
        }
    }

    private void enviarAlarmaManual() {
        try {
            if (client != null && client.isConnected()) {
                client.publish(TOPIC_ALARMA, new MqttMessage("ON".getBytes()));
                JOptionPane.showMessageDialog(this, "Alarma activada manualmente");
            } else {
                JOptionPane.showMessageDialog(this, "No conectado al broker.");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new App().setVisible(true));
    }
}