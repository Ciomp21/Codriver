package models;

import org.json.JSONException;
import org.json.JSONObject;

public class Screen {
    private String title, description, imageUrl;


    private boolean settingsVisible;
    private int id, min=0, max=1;


    public Screen (String n, int x, String d, String imgUrl, boolean settingsVis) {
        title=n;
        id=x;
        description=d;
        imageUrl=imgUrl;
        settingsVisible=settingsVis;
    }

    public String getTitle() {return title;    }

    public int getId() {
        return id;
    }
    public String getDescription() {
        return description;
    }
    public boolean isSettingsVisible() {return settingsVisible;    }
    public String getImageUrl() {return imageUrl;    }

    public void setMinMax(int min, int max) {
        this.min=min;
        this.max=max;
    }
    public String getMinMax() {
        return min + "," + max;
    }
    public String statsForJson(){
        JSONObject json = new JSONObject();
        try{

        json.put("min", min);
        json.put("max", max);
        json.put("id", id);
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
        return json.toString();
    }
}
//                 public View getView(int position, View convertView, @NonNull ViewGroup parent) {



