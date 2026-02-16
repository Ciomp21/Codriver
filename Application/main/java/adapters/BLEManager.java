package adapters;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import androidx.core.app.ActivityCompat;

import java.util.UUID;

public class BLEManager {

    private static final String TAG = "BLEManager";

    private final Context context;
    private final Activity activity;
    private final String deviceName;
    private final UUID serviceUUID;
    private final UUID charUUID;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner bleScanner;
    private BluetoothGatt bluetoothGatt;
    private BluetoothGattCharacteristic writeCharacteristic;

    private boolean isConnected = false;

    public BLEManager(Context context, Activity activity, String deviceName,
                      String serviceUUID, String charUUID) {
        this.context = context;
        this.activity = activity;
        this.deviceName = deviceName;
        this.serviceUUID = UUID.fromString(serviceUUID);
        this.charUUID = UUID.fromString(charUUID);

        BluetoothManager manager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = manager.getAdapter();
        if (bluetoothAdapter != null) {
            bleScanner = bluetoothAdapter.getBluetoothLeScanner();
        }
    }

    public void connect() {
        if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
            Log.e(TAG, "Bluetooth non disponibile o non attivo");
            return;
        }

        // Controllo permessi runtime
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(activity,
                        new String[]{Manifest.permission.BLUETOOTH_CONNECT}, 1);
                Log.e(TAG, "Permesso BLUETOOTH_CONNECT non concesso");
                return;
            }
        }

        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,
                    new String[]{Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.ACCESS_FINE_LOCATION}, 101);
            Log.e(TAG, "Permessi BLE scan non concessi");
            return;
        }

        // Avvia scansione BLE per trovare il dispositivo
        if (bleScanner != null) {
            Log.d(TAG, "Avvio scansione BLE...");
            bleScanner.startScan(scanCallback);
        }
    }

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result.getDevice();
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return;
            }
            if (device.getName() != null && device.getName().equals(deviceName)) {
                Log.d(TAG, "Trovato ESP32 BLE: " + deviceName);
                if (bleScanner != null) {
                    bleScanner.stopScan(this);
                }
                bluetoothGatt = device.connectGatt(context, false, gattCallback);
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e(TAG, "Scan BLE fallito, codice: " + errorCode);
        }
    };

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
                Log.d(TAG, "Connesso al BLE, scansionando servizi...");
                isConnected = true;
                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) return;
                gatt.discoverServices();
            } else if (newState == android.bluetooth.BluetoothProfile.STATE_DISCONNECTED) {
                Log.d(TAG, "Disconnesso dal BLE");
                isConnected = false;
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                writeCharacteristic = gatt.getService(serviceUUID).getCharacteristic(charUUID);
                if (writeCharacteristic != null) {
                    Log.d(TAG, "Characteristic trovata, pronto a scrivere");
                } else {
                    Log.e(TAG, "Characteristic BLE non trovata");
                }
            } else {
                Log.e(TAG, "Servizi BLE non scoperti correttamente");
            }
        }
    };

    // Invia comando all'ESP32
    public void sendCommand(String cmd) {
        if (writeCharacteristic != null && bluetoothGatt != null) {
            writeCharacteristic.setValue(cmd);
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) return;
            bluetoothGatt.writeCharacteristic(writeCharacteristic);
            Log.d(TAG, "Comando inviato: " + cmd);
        } else {
            Log.e(TAG, "Non pronto a inviare dati");
        }
    }

    // Chiudi connessione
    public void close() {
        if (bluetoothGatt != null) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) return;
            bluetoothGatt.close();
            bluetoothGatt = null;
            Log.d(TAG, "Connessione BLE chiusa");
        }
    }

    public boolean isConnected() {
        return bluetoothGatt != null && writeCharacteristic != null && isConnected;
    }
}
