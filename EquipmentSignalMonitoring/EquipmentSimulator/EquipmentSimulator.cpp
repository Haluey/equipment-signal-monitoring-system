#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>
#include <string>

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> signalDist(0.0, 1.0);
    std::uniform_int_distribution<int> frequencyDist(1000, 1500);
    std::uniform_real_distribution<double> temperatureDist(25.0, 40.0);

    while (true)
    {
        double signal = signalDist(gen);
        int frequency = frequencyDist(gen);
        double temperature = temperatureDist(gen);

        std::string status;

        if (temperature >= 37.0)
        {
            status = "WARNING";
        }
        else
        {
            status = "NORMAL";
        }

        std::cout << std::fixed << std::setprecision(2);

        std::cout
            << "Signal: " << signal
            << " | Frequency: " << frequency << " Hz"
            << " | Temperature: " << temperature << " C"
            << " | Status: " << status
            << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}