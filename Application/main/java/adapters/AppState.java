package adapters;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.net.HttpURLConnection;
import java.net.URL;
import org.json.JSONObject;
import android.graphics.Color;

public class AppState {

    private static AppState instance;

    // Stato globale
    private int selectedColor = 0xFFFFFF;

    private int selectedScreens = 1;

    // Handler per il ciclo
    private final Handler handler = new Handler(Looper.getMainLooper());

    private AppState() {
    }
    public String printRGB() {



        JSONObject json = new JSONObject();
        try {
        json.put("r", Color.red(selectedColor));
        json.put("g", Color.green(selectedColor));
        json.put("b", Color.blue(selectedColor));}
        catch (Exception e){}
        System.out.println("colore e :" + json);
        return json.toString();
    }


    public static AppState getInstance() {
        if (instance == null) {
            instance = new AppState();
        }
        return instance;
    }

    // Getter e setter
    public int getSelectedColor() {
        return selectedColor;
    }

    public void setSelectedColor(int color) {
        this.selectedColor = color;
    }

    public int getSelectedScreens() {
        return selectedScreens;
    }
    public void setSelectedScreens(int x) {
        this.selectedScreens = x;
    }
}