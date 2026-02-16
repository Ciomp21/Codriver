package adapters;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import androidx.core.app.ActivityCompat;

import java.io.IOException;
import java.io.OutputStream;
import java.util.Set;
import java.util.UUID;

public class BluetoothManager {

    private final Context context;
    private final Activity activity;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket bluetoothSocket;
    private OutputStream outputStream;

    private final String deviceName; // Es. "HC-05"
    private final UUID SPP_UUID = UUID.fromString("abcdef01-1234-5678-1234-5678abcdef0"); // Sostituisci con l'UUID corretto

    public BluetoothManager(Context context, Activity activity, String deviceName) {
        this.context = context;
        this.activity = activity;
        this.deviceName = deviceName;

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    // Connessione Bluetooth
    public void connect() {
        if (bluetoothAdapter == null) {
            Log.e("BluetoothManager", "Bluetooth non supportato");
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            Log.e("BluetoothManager", "Bluetooth non attivo");
            return;
        }

        // Permessi Android 12+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                Log.e("BluetoothManager", "Permesso BLUETOOTH_CONNECT non concesso");
                ActivityCompat.requestPermissions(activity,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT}, 1);
                return;
            }
        }

        // Cerca dispositivo già accoppiato
        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
        BluetoothDevice device = null;
        for (BluetoothDevice d : pairedDevices) {
            if (d.getName().equals(deviceName)) {
                device = d;
                break;
            }
        }

        if (device == null) {
            Log.e("BluetoothManager", "Dispositivo Bluetooth non trovato");
            return;
        }

        try {
            bluetoothSocket = device.createRfcommSocketToServiceRecord(SPP_UUID);
            bluetoothSocket.connect();
            outputStream = bluetoothSocket.getOutputStream();
            Log.d("BluetoothManager", "Bluetooth connesso a " + deviceName);
        } catch (IOException e) {
            e.printStackTrace();
            Log.e("BluetoothManager", "Errore connessione Bluetooth");
        }
    }

    // Invia un comando al dispositivo
    public void sendCommand(String cmd) {
        if (outputStream == null) {
            Log.e("BluetoothManager", "OutputStream non disponibile, comando non inviato");
            return;
        }

        new Thread(() -> {
            try {
                outputStream.write((cmd + "\n").getBytes());
                outputStream.flush();
                Log.d("BluetoothManager", "Comando inviato: " + cmd);
            } catch (IOException e) {
                e.printStackTrace();
                Log.e("BluetoothManager", "Errore invio comando Bluetooth");
            }
        }).start();
    }

    // Chiude la connessione
    public void close() {
        try {
            if (outputStream != null) outputStream.close();
            if (bluetoothSocket != null) bluetoothSocket.close();
            Log.d("BluetoothManager", "Connessione Bluetooth chiusa");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // Verifica se è connesso
    public boolean isConnected() {
        return bluetoothSocket != null && bluetoothSocket.isConnected();
    }
}
