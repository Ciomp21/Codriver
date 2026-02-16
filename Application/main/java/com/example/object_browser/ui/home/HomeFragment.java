package com.example.object_browser.ui.home;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import com.example.object_browser.databinding.FragmentHomeBinding;

import java.util.ArrayList;
import java.util.List;

import adapters.ScreenAdapter;
import models.Screen;

public class HomeFragment extends Fragment {

    private FragmentHomeBinding binding;
    private ScreenAdapter adapter;
    private List<Screen> items;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {

        binding = FragmentHomeBinding.inflate(inflater, container, false);

        // Lista finta di oggetti Screen
        items = new ArrayList<>();
        items.add(new Screen("Boost", 1,"Displays the boost pressure", "boost", true));
        items.add(new Screen("RPM", 2,"Displays the engine rmp", "rpm", true));
        items.add(new Screen("G-Force", 3,"Displays the acceleration as a graph", "gforce", false));
        items.add(new Screen("Liquids temp", 4,"Displays the oil and coolant temperature", "cartemp", false));
        items.add(new Screen("Air temp", 5,"displays the temperature inside the car", "airtemp", false));
        items.add(new Screen("Battery", 6,"Displays the voltage of teh battery", "battery", true));
        items.add(new Screen("Roll", 7,"Displays the car incline on the x axis", "pitch", false));
        items.add(new Screen("Pitch", 8,"Displays the car incline on the y axis", "pitch", false));

        //setup min max
        items.get(0).setMinMax(0, 2);
        items.get(1).setMinMax(0, 8000);

        // Setup RecyclerView
        adapter = new ScreenAdapter(requireContext(), items);
        binding.recyclerScreens.setLayoutManager(new LinearLayoutManager(requireContext()));
        binding.recyclerScreens.setAdapter(adapter);

        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}
