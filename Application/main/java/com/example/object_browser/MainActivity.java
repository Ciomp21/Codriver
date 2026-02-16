package com.example.object_browser;

import android.content.res.ColorStateList;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import androidx.appcompat.app.AppCompatActivity;

import com.example.object_browser.databinding.ActivityMainBinding;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.skydoves.colorpickerview.ColorPickerDialog;
import com.skydoves.colorpickerview.listeners.ColorEnvelopeListener;
import com.example.object_browser.databinding.ActivityMainBinding;

import adapters.AppState;
import adapters.BLEManager;

import androidx.fragment.app.Fragment;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Iterator;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;
    private BLEManager bleManager;

    private String colorState="{\"r\":63,\"g\":81,\"b\":181}", screenState="{\"min\":1,\"max\":2,\"id\":0}";
    private AppState appState = AppState.getInstance();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Inizializza BLEManager
        bleManager = new BLEManager(
                this,
                this,
                "Codriver",
                "12345678-1234-5678-1234-56789abcdef0",
                "abcdef01-1234-5678-1234-56789abcdef0"
        );
        bleManager.connect();

        // Runnable periodico per controllare lo stato della connessione BLE
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                if (bleManager != null && bleManager.isConnected()) {
                    Log.d("MainActivity", "BLE connesso");
                    updateConnectionStatusUI("Connesso    \uD83D\uDFE2");
                } else {
                    Log.d("MainActivity", "BLE non connesso");
                    updateConnectionStatusUI("Disconnesso   \uD83D\uDD34");
                    bleManager.connect();

                }

                binding.getRoot().postDelayed(this, 5000);
            }
        };
        runnable.run();

        binding.appBarMain.fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                showAddItemDialog();
            }
        });
    }

    private BroadcastReceiver screenEventReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {

            String message = intent.getStringExtra("message");
            if (message != null) {
                System.out.println("STATUS: " + message);
                Log.d("ScreenAdapterStatus", message);
                screenState = message;
                try {
                    sendBLECommand();
                } catch (JSONException e) {
                    throw new RuntimeException(e);
                }
            }
        }
    };

    @Override
    protected void onStart() {
        super.onStart();
        LocalBroadcastManager.getInstance(this)
                .registerReceiver(screenEventReceiver, new IntentFilter("SCREEN_BUTTON_EVENT"));
    }

    @Override
    protected void onStop() {
        super.onStop();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(screenEventReceiver);
    }

    private void showAddItemDialog() {
        new ColorPickerDialog.Builder(this, R.style.ColorPickerDialogDark)
                .setTitle("Scegli un colore")
                .setPreferenceName("MyColorPickerDialog")
                .setPositiveButton("OK", (ColorEnvelopeListener) (envelope, fromUser) -> {
                    int color = envelope.getColor();

                    // Aggiorna il FAB
                    FloatingActionButton fab = findViewById(R.id.fab);
                    fab.setBackgroundTintList(ColorStateList.valueOf(color));
                    System.out.println(color);
                    // Aggiorna lo stato globale
                    appState.setSelectedColor(color);
                    colorState = appState.printRGB();
                    try {
                        sendBLECommand();
                    } catch (JSONException e) {
                        throw new RuntimeException(e);
                    }

                })
                .setNegativeButton("Annulla", (dialogInterface, i) -> dialogInterface.dismiss())
                .attachAlphaSlideBar(true)
                .attachBrightnessSlideBar(true)
                .show();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (bleManager != null) {
            bleManager.close(); // chiude la connessione BLE
        }
    }

    // -------------------------------------------------------------------------
    //  QUI: ACCESSO A connection_status NEL HomeFragment
    // -------------------------------------------------------------------------

    private View getConnectionStatusView() {
        // recupera il NavHostFragment
        Fragment navHost = getSupportFragmentManager()
                .findFragmentById(R.id.nav_host_fragment_content_main);

        if (navHost == null) return null;

        // recupera il fragment attualmente visibile
        List<Fragment> fragments = navHost.getChildFragmentManager().getFragments();
        if (fragments.isEmpty()) return null;

        Fragment currentFragment = fragments.get(0);
        if (currentFragment.getView() == null) return null;

        // finalmente accedi alla view del fragment
        return currentFragment.getView().findViewById(R.id.connectionStatus);
    }

    private void updateConnectionStatusUI(String text) {
        View cs = getConnectionStatusView();
        if (cs == null) return;  // fragment non ancora caricato

        View txtStatus = cs.findViewById(R.id.description); // dentro connection_status.xml
        if (txtStatus instanceof android.widget.TextView) {
            ((android.widget.TextView) txtStatus).setText(text);
        }
    }

    private void sendBLECommand() throws JSONException {

        // parse
        JSONObject json1 = new JSONObject(colorState);
        JSONObject json2 = new JSONObject(screenState);

// merge (json2 sovrascrive json1 se chiavi uguali)
        Iterator<String> keys = json2.keys();
        while (keys.hasNext()) {
            String key = keys.next();
            json1.put(key, json2.get(key));
        }

// risultato come stringa
        String command = json1.toString();

        System.out.println("Comando BLE: " + command);
        if (bleManager != null && bleManager.isConnected()) {
            bleManager.sendCommand(command);
            Log.d("MainActivity", "Comando BLE inviato: " + command);
        } else {
            Log.d("MainActivity", "Impossibile inviare comando, BLE non connesso"+ command);
        }
    }
}
