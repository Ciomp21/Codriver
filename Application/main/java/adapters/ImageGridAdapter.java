package adapters;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;

import java.util.List;

import models.Screen;

public class ImageGridAdapter extends BaseAdapter {
    private final Context context;
    private final List<Screen> items;

    public ImageGridAdapter(Context context, List<Screen> items) {
        this.context = context;
        this.items = items;
    }

    @Override
    public int getCount() { return items.size(); }

    @Override
    public Object getItem(int position) { return items.get(position); }

    @Override
    public long getItemId(int position) { return position; }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ImageView imageView;
        if (convertView == null) {
            imageView = new ImageView(context);
            imageView.setLayoutParams(new ViewGroup.LayoutParams(250, 250));
            imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
        } else {
            imageView = (ImageView) convertView;
        }

        // Ottieni l'ID della risorsa drawable dal nome
        int resId = context.getResources().getIdentifier(
                items.get(position).getImageUrl(), "drawable", context.getPackageName());

        imageView.setImageResource(resId);

        return imageView;
    }
}
