#include <chrono>
#include <cmath>
#include <cstdint>
#include <dpu>
#include <iostream>
#include <ostream>
#include <random>
#include <unistd.h>

using namespace dpu;

void
generate_initial_points(std::vector<std::vector<float>> &attributes);

void
pick_initial_centers(std::vector<std::vector<float>> &attributes,
                     std::vector<float> &current_cluster_centers);

int
main(int argc, char **argv)
{
    // IN
    std::vector<std::vector<float>> attributes(
        N_DPUS, std::vector<float>(NUM_OBJECTS_PER_DPU * NUM_ATTRIBUTES));

    std::vector<float> current_cluster_centers(N_CLUSTERS * NUM_ATTRIBUTES);

    // OUT
    std::vector<std::vector<float>> round_cluster_centers(
        N_DPUS, std::vector<float>(N_CLUSTERS * NUM_ATTRIBUTES));

    std::vector<std::vector<std::uint32_t>> round_cluster_centers_len(
        N_DPUS, std::vector<std::uint32_t>(N_CLUSTERS));

    std::vector<std::vector<std::uint64_t>> agregated_delta(
        N_DPUS, std::vector<std::uint64_t>(1, 0));

    // LOCAL
    std::vector<std::uint32_t> agregated_cluster_centers_len(N_CLUSTERS);
    double total_time = 0;
    int loop;

    try
    {
        auto system = DpuSet::allocate(N_DPUS);

        system.load("kmeans/kmeans");

        generate_initial_points(attributes);

        system.copy("attributes", attributes);

        auto start = std::chrono::steady_clock::now();

        // Rabdomly pick initial centers
        pick_initial_centers(attributes, current_cluster_centers);

        do
        {
            // IN: Copy current centers
            system.copy("current_cluster_centers", current_cluster_centers);

            // Execute
            system.exec();

            // system.log(std::cout);

            // OUT: Copy (agregated) new centers
            system.copy(round_cluster_centers, "local_cluster_centers");

            // OUT: Copy centers len
            system.copy(round_cluster_centers_len, "local_centers_len");

            // OUT: Copy delta
            system.copy(agregated_delta, "agregated_delta");

            // Compute new centers
            for (int i = 0; i < N_CLUSTERS * NUM_ATTRIBUTES; ++i)
            {
                current_cluster_centers[i] = 0;
            }

            for (int i = 0; i < N_CLUSTERS; ++i)
            {
                agregated_cluster_centers_len[i] = 0;
            }

            for (int j = 0; j < N_CLUSTERS; ++j)
            {
                for (int i = 0; i < N_DPUS; ++i)
                {
                    for (int c = 0; c < NUM_ATTRIBUTES; ++c)
                    {
                        current_cluster_centers[(j * NUM_ATTRIBUTES) + c] +=
                            round_cluster_centers[i][(j * NUM_ATTRIBUTES) + c];
                    }
                    agregated_cluster_centers_len[j] += round_cluster_centers_len[i][j];
                }
            }

            for (int i = 0; i < N_CLUSTERS; ++i)
            {
                if (agregated_cluster_centers_len[i] == 0)
                {
                    continue;
                }

                for (int j = 0; j < NUM_ATTRIBUTES; ++j)
                {
                    current_cluster_centers[(i * NUM_ATTRIBUTES) + j] /=
                        agregated_cluster_centers_len[i];
                }
            }

            for (int i = 0; i < N_CLUSTERS; ++i)
            {
                for (int j = 0; j < NUM_ATTRIBUTES; ++j)
                {
                    std::cout << current_cluster_centers[(i * NUM_ATTRIBUTES) + j]
                              << ", ";
                }
                std::cout << "-> " << agregated_cluster_centers_len[i] << std::endl;
            }
            // } while ((delta > THRESHOLD) && (loop++ < 500));
        } while (0);

        auto end = std::chrono::steady_clock::now();

        total_time +=
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // std::cout << (double)nThreads.front().front() << "\t" << N_DPUS << "\t"
        //           << total_time << std::endl;
    }
    catch (const DpuError &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

void
generate_initial_points(std::vector<std::vector<float>> &attributes)
{
    float tmp_centers[GENERATE_N_CENTERS][NUM_ATTRIBUTES];
    float sigma;
    int tmp_center;

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> f_rand(0, 1);
    std::uniform_int_distribution<std::mt19937::result_type> d_rand(
        0, GENERATE_N_CENTERS - 1);

    sigma = pow(((float)1 / GENERATE_N_CENTERS), 3);
    std::normal_distribution<> normal_dist(0, sigma);

    for (int i = 0; i < GENERATE_N_CENTERS; ++i)
    {
        for (int j = 0; j < NUM_ATTRIBUTES; ++j)
        {
            tmp_centers[i][j] = f_rand(rng);
        }
    }

    for (int i = 0; i < N_DPUS; ++i)
    {
        for (int c = 0; c < NUM_OBJECTS_PER_DPU; ++c)
        {
            tmp_center = d_rand(rng);
            for (int j = 0; j < NUM_ATTRIBUTES; ++j)
            {
                attributes[i][(c * NUM_ATTRIBUTES) + j] =
                    tmp_centers[tmp_center][j] + normal_dist(rng);
            }
        }
    }
}

void
pick_initial_centers(std::vector<std::vector<float>> &attributes,
                     std::vector<float> &current_cluster_centers)
{
    int dpu, point;

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<> d_rand_dpu(0, N_DPUS - 1);
    std::uniform_int_distribution<> d_rand_point(0, NUM_OBJECTS_PER_DPU - 1);

    for (int i = 0; i < N_CLUSTERS; ++i)
    {
        dpu = d_rand_dpu(rng);
        point = d_rand_point(rng);
        for (int j = 0; j < NUM_ATTRIBUTES; ++j)
        {
            current_cluster_centers[(i * NUM_ATTRIBUTES) + j] =
                attributes[dpu][(point * NUM_ATTRIBUTES) + j];
        }
    }
}
