package adapters;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.recyclerview.widget.RecyclerView;

import com.example.object_browser.R;

import java.util.ArrayList;
import java.util.List;

import models.Screen;

public class ScreenAdapter extends RecyclerView.Adapter<ScreenAdapter.ViewHolder> {

    private final Context context;
    private final List<Screen> items;
    private final AppState appState = AppState.getInstance();

    public ScreenAdapter(Context context, List<Screen> items) {
        this.context = context;
        this.items = items;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(context).inflate(R.layout.screen, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {

        // Prendi lo schermo selezionato
        Screen screen = items.get(appState.getSelectedScreens());

        // Imposta titolo e descrizione
        holder.textTitle.setText(screen.getTitle());
        holder.description.setText(screen.getDescription());

        // Imposta visibilità del bottone impostazioni
        holder.button.setVisibility(screen.isSettingsVisible() ? View.VISIBLE : View.GONE);

        // 🔹 Imposta immagine corrente sul pulsante
        int resId = context.getResources()
                .getIdentifier(screen.getImageUrl(), "drawable", context.getPackageName());
        if (resId != 0) {
            holder.buttonAction.setImageResource(resId);
        }

        // 🔹 Bottone cambio immagine (Grid dialog)
        holder.buttonAction.setOnClickListener(v -> {

            AlertDialog.Builder builder = new AlertDialog.Builder(context);
            View dialogView = LayoutInflater.from(context).inflate(R.layout.dialog_grid, null);
            GridView gridView = dialogView.findViewById(R.id.gridView);

            // Lista di tutte le immagini
            List<Integer> imageIds = new ArrayList<>();
            for (Screen s : items) {
                int id = context.getResources()
                        .getIdentifier(s.getImageUrl(), "drawable", context.getPackageName());
                if (id != 0) imageIds.add(id);
            }

            // Adapter per la GridView
            ArrayAdapter<Integer> adapter = new ArrayAdapter<Integer>(context, 0, imageIds) {
                @NonNull
                @Override
                public View getView(int pos, View convertView, @NonNull ViewGroup parent) {
                    ImageView imageView;
                    if (convertView == null) {
                        imageView = new ImageView(context);
                        int size = (int) (60 * context.getResources().getDisplayMetrics().density);
                        imageView.setLayoutParams(new GridView.LayoutParams(size, size));
                        imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
                        imageView.setPadding(8, 8, 8, 8);
                    } else {
                        imageView = (ImageView) convertView;
                    }
                    imageView.setImageResource(getItem(pos));
                    return imageView;
                }
            };

            gridView.setAdapter(adapter);

            AlertDialog dialog = builder.setView(dialogView).create();

            gridView.setOnItemClickListener((parent1, view1, pos, id1) -> {

                // Aggiorna lo schermo selezionato nello stato globale
                appState.setSelectedScreens(pos);

                // Aggiorna il pulsante con l'immagine scelta
                holder.buttonAction.setImageResource(imageIds.get(pos));

                // Notifica il RecyclerView (anche se c'è solo un item)
                notifyItemChanged(0);

                // Invia broadcast con i dati dello schermo
                Intent intent = new Intent("SCREEN_BUTTON_EVENT");
                intent.putExtra("message", items.get(appState.getSelectedScreens()).statsForJson());
                LocalBroadcastManager.getInstance(context).sendBroadcast(intent);

                dialog.dismiss();
            });

            dialog.show();
        });

        // 🔹 Bottone Min/Max
        holder.button.setOnClickListener(v -> {

            AlertDialog.Builder builder = new AlertDialog.Builder(context, R.style.ColorPickerDialogDark);
            View dialogView = LayoutInflater.from(context).inflate(R.layout.dialog_mix_max, null);
            builder.setView(dialogView);

            EditText editMin = dialogView.findViewById(R.id.editMin);
            EditText editMax = dialogView.findViewById(R.id.editMax);

            builder.setPositiveButton("OK", (dialog, which) -> {
                try {
                    int min = Integer.parseInt(editMin.getText().toString());
                    int max = Integer.parseInt(editMax.getText().toString());

                    if (min > max) {
                        Toast.makeText(context, "Min non può essere maggiore di Max", Toast.LENGTH_SHORT).show();
                        return;
                    }

                    screen.setMinMax(min, max);
                    notifyItemChanged(0);

                } catch (Exception e) {
                    Toast.makeText(context, "Valori non validi", Toast.LENGTH_SHORT).show();
                }
            });

            builder.setNegativeButton("Annulla", (dialog, which) -> dialog.dismiss());

            builder.create().show();
        });
    }
    @Override
    public int getItemCount() {
        return 1; // SOLO UNO
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        TextView textTitle;
        TextView description;
        ImageButton buttonAction; // <-- cambiato
        ImageButton button;

        public ViewHolder(@NonNull View itemView) {
            super(itemView);
            textTitle = itemView.findViewById(R.id.textTitle);
            description = itemView.findViewById(R.id.description);
            buttonAction = itemView.findViewById(R.id.buttonAction); // ok così
            button = itemView.findViewById(R.id.button);
        }
    }
}
