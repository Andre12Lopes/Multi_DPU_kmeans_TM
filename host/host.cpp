#include <chrono>
#include <dpu>
#include <iostream>
#include <ostream>
#include <random>
#include <unistd.h>
#include <cmath>

using namespace dpu;

void create_bach(std::vector<std::vector<float>> &attributes);

int main(int argc, char **argv)
{
    std::vector<std::vector<float>> attributes(N_DPUS, std::vector<float>(NUM_OBJECTS_PER_DPU * NUM_ATTRIBUTES));
    std::vector<std::vector<uint32_t>> nThreads(1);
    double total_time = 0;

    try
    {
        auto system = DpuSet::allocate(N_DPUS);

        system.load("bank/bank");

        create_bach(attributes);

        system.copy("attributes", attributes);

        auto start = std::chrono::steady_clock::now();

        

        auto end = std::chrono::steady_clock::now();

        total_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // std::vector<std::vector<int>> bach(system.dpus().size(), std::vector<int>(BACH_SIZE * NR_TASKLETS));
    //     for (int i = 0; i < N_BACHES; ++i)
    //     {
    //         create_bach(system, bach);

    //         auto start = std::chrono::steady_clock::now();
        
    //         system.copy("bach", bach);

    //         system.exec();

    //         auto end = std::chrono::steady_clock::now();

    //         total_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //     }

        auto dpu = system.dpus()[0];
        nThreads.front().resize(1);
        dpu->copy(nThreads, "n_tasklets");

        std::cout << (double) nThreads.front().front() << "\t"
                  << N_DPUS << "\t"
                  << total_time << std::endl;
    }
    catch (const DpuError &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

void create_bach(std::vector<std::vector<float>> &attributes)
{
    std::vector<std::vector<float>> tmp_centers(GENERATE_N_CENTERS, std::vector<float>(NUM_ATTRIBUTES));

    std::random_device dev;
    std::mt19937 rng(dev());

    // std::uniform_int_distribution<std::mt19937::result_type> rand(0, GENERATE_N_CENTERS);
    std::uniform_real_distribution<> f_rand(0, 1);

    // float sigma;
    // float noise;
    // int center;

    // for (unsigned i = 0; i < GENERATE_N_CENTERS; ++i)
    // {
    //     for (int j = 0; j < NUM_ATTRIBUTES; ++j)
    //     {
    //         tmp_centers[i][j] = f_rand(rng);
    //     }
    // }

    // sigma = powf((1 / GENERATE_N_CENTERS), 3);

    // for (unsigned i = 0; i < N_DPUS; ++i)
    // {
    //     for (int j = 0; j < NUM_OBJECTS_PER_DPU; ++j)
    //     {
    //         center = rand(rng);
    //         for (int c = 0; c < NUM_ATTRIBUTES; ++c)
    //         {
    //             noise = ;
    //             attributes[i][(j * NUM_ATTRIBUTES) + c] = tmp_centers[center][c] + noise;
    //         }
    //     }
    // }

    for (unsigned i = 0; i < N_DPUS; ++i)
    {
        for (int j = 0; j < NUM_OBJECTS_PER_DPU * NUM_ATTRIBUTES; ++j)
        {
            attributes[i][j] = f_rand(rng);
        }
    }    
}