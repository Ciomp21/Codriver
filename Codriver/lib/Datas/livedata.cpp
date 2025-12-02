#include "livedata.h"

struct SlidingWindow
{
private:
    int windowSize;
    float *data;
    int index;
    int count;

public:
    SlidingWindow(int size) : windowSize(size), index(0), count(0)
    {
        data = new float[windowSize];
        for (int i = 0; i < windowSize; ++i)
            data[i] = 0.0f;
    }
    ~SlidingWindow()
    {
        delete[] data;
    }

    bool full() const
    {
        return count == windowSize;
    }

    void clear()
    {
        index = 0;
        count = 0;
        for (int i = 0; i < windowSize; ++i)
            data[i] = 0.0f;
    }

    void addData(float value)
    {
        data[index] = value;
        index = (index + 1) % windowSize;
        if (count < windowSize)
            count++;
    }

    float getAverage() const
    {
        float sum = 0.0f;
        for (int i = 0; i < count; ++i)
            sum += data[i];
        return (count > 0) ? (sum / count) : 0.0f;
    }
};

SlidingWindow speedWindow(50); // 50-sample sliding window for speed
SlidingWindow mafWindow(50);   // 50-sample sliding window for MAF

SlidingWindow fuelWindow(20); // 20-sample sliding window for fuel consumption

SlidingWindow accelWindowX(20); // 20-sample sliding window for acceleration force
SlidingWindow accelWindowY(20); // 20-sample sliding window for cornering force
SlidingWindow accelWindowZ(20); // 20-sample sliding window for braking force

unsigned long lastUpdateTimeTrip = 0;

void updateTripInfo(Values *liveData)
{
    // Trip distance could be computed by integrating speed over time
    unsigned long currentTime = millis();

    if (lastUpdateTimeTrip != 0)
    {
        float deltaTimeHours = (currentTime - lastUpdateTimeTrip) / 3600000.0f; // Convert ms to hours

        // Approximate trip distance increment = speed * time (Fast enough for small intervals)
        liveData->tripDistance += liveData->speed * deltaTimeHours; // distance = speed * time
    }

    // Trip fuel used = integrate fuel consumption over distance (Just an approximation here but good enough)
    liveData->tripFuelUsed += liveData->fuelConsumption * liveData->speed * ((currentTime - lastUpdateTimeTrip) / 3600000.0f); // L/100km * km = L

    // Trip duration in seconds
    liveData->tripDuration += (currentTime - lastUpdateTimeTrip) / 1000.0f; // Convert ms to seconds

    if (liveData->tripDuration > 0)
    {
        liveData->averageSpeed = liveData->tripDistance / (liveData->tripDuration / 3600.0f);          // km/h
        liveData->averageFuelConsumption = (liveData->tripFuelUsed / liveData->tripDistance) * 100.0f; // L/100km
    }

    // Update Last Update Time
    lastUpdateTimeTrip = currentTime;
}

void RecomputeDerivedData(Values *liveData)
{
    // Compute any derived data from the raw sensor/OBD2 data

    // Smooth out speed and maf consumption using sliding window average
    speedWindow.addData(liveData->speed);
    mafWindow.addData(liveData->maf);

    if (liveData->accelX == 0.0f)
    {
        liveData->fuelConsumption = 2.0f; // Idle uses ~2 L/h
    }
    else if (speedWindow.full() && mafWindow.full() && speedWindow.getAverage() > 1.0f)
    {
        liveData->fuelConsumption = (mafWindow.getAverage() * 3600.0f) / (14.7f * 720.0f * speedWindow.getAverage()); // L/100km
        fuelWindow.addData(liveData->fuelConsumption);
        liveData->fuelConsumption = fuelWindow.getAverage();
        speedWindow.clear();
        mafWindow.clear();
    }

    // Estimate range based on current fuel level and consumption
    if (fuelWindow.full())
    {
        liveData->extimatedRange = (liveData->fuelLevel / fuelWindow.getAverage()) * 100.0f * 100.0f; // in km
    }

    // Update Engine Load as percentage
    if (liveData->rpm > 0)
    {
        liveData->engineLoad = (liveData->maf / (0.1f * liveData->rpm)) * 100.0f; // Simplified estimation
        if (liveData->engineLoad > 100.0f)
            liveData->engineLoad = 100.0f;
    }
    else
    {
        liveData->engineLoad = 0.0f;
    }

    // Suggest gear based on speed and rpm (very simplified)
    if (liveData->rpm > 2000)
    {
        liveData->gearSuggestion = liveData->gear + 1;
    }

    if (liveData->rpm < 1200 && liveData->gear > 1)
    {
        liveData->gearSuggestion = liveData->gear - 1;
    }

    // Update trip information every 500 ms
    if (millis() - lastUpdateTimeTrip >= delayTrip)
    {
        updateTripInfo(liveData);
    }
}

void InitLiveData(Values *liveData)
{
    // Initialize live data structure
    liveData->speed = 0.0f;
    liveData->maf = 0.0f;
    liveData->fuelConsumption = 0.0f;
    liveData->tripDistance = 0.0f;
    liveData->tripFuelUsed = 0.0f;
    liveData->tripDuration = 0.0f;
    liveData->averageSpeed = 0.0f;
    liveData->averageFuelConsumption = 0.0f;
}