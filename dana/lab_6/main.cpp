#include <iostream>
#include <cmath>
#include <iomanip>
#include <pthread.h>
#include <omp.h>
#include <chrono>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_NODES 64
#define INF_COST 1e18

using namespace std;

static inline uint64_t xorshift64star(uint64_t* s) 
{
    uint64_t x = *s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *s = x;
    return x * 2685821657736338717ULL;
}

static inline double rand01(uint64_t* s) 
{
    return (xorshift64star(s) >> 11) * (1.0 / 9007199254740992.0);
}

static inline uint64_t now_ns() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}


struct ACOParams 
{
    int iterations;
    int num_ants;
    double alpha;
    double beta;
    double rho;
    double Q;
    double tau0;
    double tau_min;
    double tau_max;
    uint64_t seed;
};


void build_vienna_graph(int* outN, double** outTime, const char*** outNames) 
{
    int N = 15;
    double* timeMat = (double*)calloc(N * N, sizeof(double));
    const char** names = (const char**)malloc(sizeof(char*) * N);

    names[0]  = "Karlsplatz";
    names[1]  = "Stephansplatz";
    names[2]  = "Schwedenplatz";
    names[3]  = "Praterstern";
    names[4]  = "Landstrasse (Wien Mitte)";
    names[5]  = "Stadtpark";
    names[6]  = "Schottenring";
    names[7]  = "Volkstheater";
    names[8]  = "Westbahnhof";
    names[9]  = "Neubaugasse";
    names[10] = "Herrengasse";
    names[11] = "Zieglergasse";
    names[12] = "Taubstummengasse";
    names[13] = "Nestroyplatz";
    names[14] = "Messe-Prater";

    auto add_u = [&](int a, int b, double w) 
    {
        timeMat[a * N + b] = w;
        timeMat[b * N + a] = w;
    };

    add_u(0, 1, 2);
    add_u(1, 2, 2);
    add_u(2, 13, 2);
    add_u(13, 3, 2);
    add_u(12, 0, 2);
    add_u(8, 11, 2);
    add_u(11, 9, 2);
    add_u(9, 10, 2);
    add_u(10, 1, 2);
    add_u(1, 4, 2);
    add_u(0, 5, 2);
    add_u(5, 4, 2);
    add_u(4, 2, 2);
    add_u(2, 6, 2);
    add_u(0, 7, 2);
    add_u(7, 6, 2);
    add_u(6, 3, 2);
    add_u(3, 14, 2);

    *outN = N;
    *outTime = timeMat;
    *outNames = names;
}


void build_eta(int N, const double* timeMat, double* eta) 
{
    for (int i = 0; i < N; ++i) 
    {
        for (int j = 0; j < N; ++j) 
        {
            double w = timeMat[i * N + j];
            eta[i * N + j] = (w > 0.0) ? (1.0 / w) : 0.0;
        }
    }
}

void init_pheromones(int N, const double* timeMat, double* pher, double tau0) 
{
    for (int i = 0; i < N; ++i) 
    {
        for (int j = 0; j < N; ++j) 
        {
            pher[i * N + j] = (timeMat[i * N + j] > 0.0) ? tau0 : 0.0;
        }
    }
}

static inline void clamp_matrix(int N, double* mat, double lo, double hi) 
{
    if (lo <= 0.0 && hi <= 0.0) return;
    
    for (int i = 0; i < N * N; ++i) 
    {
        if (mat[i] <= 0.0) continue;
        if (lo > 0.0 && mat[i] < lo) mat[i] = lo;
        if (hi > 0.0 && mat[i] > hi) mat[i] = hi;
    }
}


struct AntPath 
{
    int nodes[MAX_NODES];
    int len;
    double cost;
    int success;
};

static inline int choose_next_node(
    int N, int u, const double* timeMat, const double* pher, const double* eta,
    const int* visited, double alpha, double beta, uint64_t* rng, int* candBuf, double* probBuf, int* outCount ) 
{
    int c = 0;
    for (int v = 0; v < N; ++v) 
    {
        if (timeMat[u * N + v] > 0.0 && !visited[v]) 
        {
            candBuf[c++] = v;
        }
    }

    if (c == 0) { *outCount = 0; return -1; }

    double sumP = 0.0;
    
    for (int i = 0; i < c; ++i) 
    {
        int v = candBuf[i];
        double tau = pher[u * N + v];
        double et  = eta[u * N + v];
        double w = pow(tau, alpha) + 1e-18;
    
        if (beta > 0.0) w *= pow(et, beta);
    
        probBuf[i] = w;
        sumP += w;
    }
    if (sumP <= 0.0) 
    {
        int k = (int)floor(rand01(rng) * c);

        if (k >= c) k = c - 1;

        *outCount = c;

        return candBuf[k];
    }

    double r = rand01(rng) * sumP;
    double acc = 0.0;

    for (int i = 0; i < c; ++i) 
    {
        acc += probBuf[i];
    
        if (r <= acc) 
        {
            *outCount = c;
            return candBuf[i];
        }
    }

    *outCount = c;
    
    return candBuf[c - 1];
}

void construct_ant_path(
    int N, const double* timeMat, const double* pher, const double* eta,
    int start, int target, double alpha, double beta, uint64_t* rng,
    AntPath* out) 
{
    int visited[MAX_NODES] = {0};
    int candBuf[MAX_NODES];
    double probBuf[MAX_NODES];

    out->len = 1;
    out->nodes[0] = start;
    out->cost = 0.0;
    out->success = 0;

    visited[start] = 1;
    int u = start;

    for (int step = 0; step < N - 1; ++step) 
    {
        if (u == target) 
        {
            out->success = 1;
            return;
        }

        int candCount = 0;
        int v = choose_next_node(N, u, timeMat, pher, eta, visited, alpha, beta, rng, candBuf, probBuf, &candCount);
        
        if (candCount == 0 || v < 0) 
        {
            out->success = 0;
            out->cost = INF_COST;
            return;
        }

        out->nodes[out->len++] = v;
        out->cost += timeMat[u * N + v];
        visited[v] = 1;
        u = v;
    }

    if (u == target) 
    {
        out->success = 1;
    } 
    else 
    {
        out->success = 0;
        out->cost = INF_COST;
    }
}

void evaporate_and_deposit(int N, double* pher, double rho, const double* delta, double tau_min, double tau_max) 
{
    for (int i = 0; i < N * N; ++i) 
    {
        if (pher[i] > 0.0) 
        {
            pher[i] = (1.0 - rho) * pher[i] + delta[i];
        }
    }

    clamp_matrix(N, pher, tau_min, tau_max);
}

void aco_shortest_path_sequential(
    int N, const double* timeMat, int start, int target, const ACOParams* p,
    int* bestPathOut, int* bestLenOut, double* bestCostOut) 
{
    double* eta = (double*)malloc(sizeof(double) * N * N);
    double* pher = (double*)malloc(sizeof(double) * N * N);
    double* delta = (double*)malloc(sizeof(double) * N * N);

    build_eta(N, timeMat, eta);
    init_pheromones(N, timeMat, pher, p->tau0);

    int* pathBuf = (int*)malloc(sizeof(int) * N);
    double bestCost = INF_COST;
    int bestLen = 0;

    uint64_t rng = (p->seed ? p->seed : (uint64_t)now_ns()) ^ 0x9e3779b97f4a7c15ULL;

    for (int iter = 0; iter < p->iterations; ++iter) 
    {
        memset(delta, 0, sizeof(double) * N * N);

        for (int a = 0; a < p->num_ants; ++a) 
        {
            AntPath ap;
            construct_ant_path(N, timeMat, pher, eta, start, target, p->alpha, p->beta, &rng, &ap);
            
            if (ap.success && ap.cost < INF_COST * 0.5) 
            {
                double add = (ap.cost > 0.0) ? (p->Q / ap.cost) : 0.0;
            
                for (int i = 0; i + 1 < ap.len; ++i) 
                {
                    int u = ap.nodes[i];
                    int v = ap.nodes[i + 1];
                    delta[u * N + v] += add;
                    delta[v * N + u] += add;
                }

                if (ap.cost < bestCost) 
                {
                    bestCost = ap.cost;
                    bestLen = ap.len;
                
                    for (int i = 0; i < ap.len; ++i) bestPathOut[i] = ap.nodes[i];
                
                    *bestLenOut = bestLen;
                    *bestCostOut = bestCost;
                }
            }
        }

        evaporate_and_deposit(N, pher, p->rho, delta, p->tau_min, p->tau_max);
    }

    free(eta);
    free(pher);
    free(delta);
    free(pathBuf);
}

struct ThreadShared 
{
    int N;
    const double* timeMat;
    const double* eta;
    double* pher;
    int start;
    int target;
    ACOParams params;

    pthread_barrier_t barrier;
    int num_threads;
    int ants_per_thread;
    double** deltas;
    double* thread_best_cost;
    int* thread_best_len;
    int* thread_best_path;
    double* global_best_cost;
    int* global_best_len;
    int* global_best_path;
};

struct ThreadArg 
{
    int tid;
    ThreadShared* sh;
    uint64_t rng;
};

static void* aco_worker(void* arg) 
{
    ThreadArg* ta = (ThreadArg*)arg;
    ThreadShared* sh = ta->sh;
    int tid = ta->tid;
    uint64_t rng = ta->rng;
    int N = sh->N;
    const double* timeMat = sh->timeMat;
    const double* eta = sh->eta;
    double* pher = sh->pher;
    const ACOParams* p = &sh->params;
    double* local_delta = sh->deltas[tid];
    double* tbest_cost = sh->thread_best_cost;
    int* tbest_len = sh->thread_best_len;
    int* tbest_path = sh->thread_best_path + tid * N;

    for (int iter = 0; iter < p->iterations; ++iter) 
    {
        tbest_cost[tid] = INF_COST;
        tbest_len[tid] = 0;

        memset(local_delta, 0, sizeof(double) * N * N);

        for (int a = 0; a < sh->ants_per_thread; ++a) 
        {
            AntPath ap;
            construct_ant_path(N, timeMat, pher, eta, sh->start, sh->target, p->alpha, p->beta, &rng, &ap);
        
            if (ap.success && ap.cost < INF_COST * 0.5) 
            {
                double add = (ap.cost > 0.0) ? (p->Q / ap.cost) : 0.0;
            
                for (int i = 0; i + 1 < ap.len; ++i) 
                {
                    int u = ap.nodes[i];
                    int v = ap.nodes[i + 1];
                    local_delta[u * N + v] += add;
                    local_delta[v * N + u] += add;
                }
                
                if (ap.cost < tbest_cost[tid]) 
                {
                    tbest_cost[tid] = ap.cost;
                    tbest_len[tid] = ap.len;
                
                    for (int i = 0; i < ap.len; ++i) tbest_path[i] = ap.nodes[i];
                }
            }
        }

        pthread_barrier_wait(&sh->barrier);

        if (tid == 0) 
        {
            for (int i = 0; i < N * N; ++i) 
            {
                if (pher[i] > 0.0) pher[i] = (1.0 - p->rho) * pher[i];
            }

            for (int t = 0; t < sh->num_threads; ++t) 
            {
                double* d = sh->deltas[t];
            
                for (int i = 0; i < N * N; ++i) 
                {
                    pher[i] += d[i];
                }
            }

            clamp_matrix(N, pher, p->tau_min, p->tau_max);

            for (int t = 0; t < sh->num_threads; ++t) 
            {
                if (tbest_cost[t] < *sh->global_best_cost) 
                {
                    *sh->global_best_cost = tbest_cost[t];
                    *sh->global_best_len = tbest_len[t];
                    int* src = sh->thread_best_path + t * N;
                
                    for (int i = 0; i < *sh->global_best_len; ++i) 
                    {
                        sh->global_best_path[i] = src[i];
                    }
                }
            }
        }

        pthread_barrier_wait(&sh->barrier);
        memset(local_delta, 0, sizeof(double) * N * N);
    }

    return NULL;
}

void aco_shortest_path_parallel_pthreads(
    int N, const double* timeMat, int start, int target, const ACOParams* p,
    int num_threads,
    int* bestPathOut, int* bestLenOut, double* bestCostOut) 
{
    if (num_threads < 1) num_threads = 1;
    if (num_threads > 64) num_threads = 64;

    ThreadShared sh;
    sh.N = N;
    sh.timeMat = timeMat;
    sh.start = start;
    sh.target = target;
    sh.params = *p;
    sh.num_threads = num_threads;
    sh.ants_per_thread = p->num_ants / num_threads;

    if (sh.ants_per_thread < 1) sh.ants_per_thread = 1;

    double* eta = (double*)malloc(sizeof(double) * N * N);
    double* pher = (double*)malloc(sizeof(double) * N * N);
    build_eta(N, timeMat, eta);
    init_pheromones(N, timeMat, pher, p->tau0);
    sh.eta = eta;
    sh.pher = pher;
    sh.deltas = (double**)malloc(sizeof(double*) * num_threads);

    for (int t = 0; t < num_threads; ++t) 
    {
        sh.deltas[t] = (double*)calloc(N * N, sizeof(double));
    }

    sh.thread_best_cost = (double*)malloc(sizeof(double) * num_threads);
    sh.thread_best_len  = (int*)malloc(sizeof(int) * num_threads);
    sh.thread_best_path = (int*)malloc(sizeof(int) * num_threads * N);

    double global_best_cost = INF_COST;
    int global_best_len = 0;
    int* global_best_path = (int*)malloc(sizeof(int) * N);

    sh.global_best_cost = &global_best_cost;
    sh.global_best_len = &global_best_len;
    sh.global_best_path = global_best_path;

    pthread_barrier_init(&sh.barrier, NULL, num_threads);

    pthread_t* th = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    ThreadArg* args = (ThreadArg*)malloc(sizeof(ThreadArg) * num_threads);
    uint64_t base_seed = (p->seed ? p->seed : (uint64_t)now_ns()) ^ 0x517cc1b727220a95ULL;

    for (int t = 0; t < num_threads; ++t) 
    {
        args[t].tid = t;
        args[t].sh = &sh;
        args[t].rng = base_seed + 0x9e3779b97f4a7c15ULL * (uint64_t)t;
        pthread_create(&th[t], NULL, aco_worker, &args[t]);
    }

    for (int t = 0; t < num_threads; ++t) 
    {
        pthread_join(th[t], NULL);
    }

    *bestCostOut = global_best_cost;
    *bestLenOut = global_best_len;
    for (int i = 0; i < global_best_len; ++i) bestPathOut[i] = global_best_path[i];

    pthread_barrier_destroy(&sh.barrier);

    for (int t = 0; t < num_threads; ++t) free(sh.deltas[t]);

    free(sh.deltas);
    free(sh.thread_best_cost);
    free(sh.thread_best_len);
    free(sh.thread_best_path);
    free(global_best_path);
    free(eta);
    free(pher);
    free(th);
    free(args);
}


static void print_path(const char** names, const int* path, int len, double cost) 
{
    printf("Лучший путь (%.2f мин): ", cost);

    for (int i = 0; i < len; ++i)
    {
        printf("%s", names[path[i]]);
    
        if (i + 1 < len) printf(" -> ");
    }

    printf("\n");
}

double f(double x) 
{
    if (x >= 0 && x <= 0.6) 
        return 1.0 / (1.0 + 25.0 * x * x);
    else if (x > 0.6 && x <= 1.6) 
        return (x + 2.0 * pow(x, 4)) * sin(x * x);
    else 
        return 0.0;
}

double trapezoidal_sequential(double a, double b, int n) 
{
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    
    for (int i = 1; i < n; i++) 
    {
        double x = a + i * h;
        sum += f(x);
    }
    
    return sum * h;
}

struct ThreadData 
{
    double a;
    double b;
    int n;
    int thread_id;
    int num_threads;
    double partial_sum;
};

void* trapezoidal_thread(void* arg) 
{
    ThreadData* data = (ThreadData*)arg;
    double h = (data->b - data->a) / data->n;
    data->partial_sum = 0.0;
    
    for (int i = data->thread_id + 1; i < data->n; i += data->num_threads) 
    {
        double x = data->a + i * h;
        data->partial_sum += f(x);
    }
    
    return nullptr;
}

double trapezoidal_pthreads(double a, double b, int n, int num_threads) 
{
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    
    for (int i = 0; i < num_threads; i++) 
    {
        thread_data[i] = {a, b, n, i, num_threads, 0.0};
        pthread_create(&threads[i], nullptr, trapezoidal_thread, &thread_data[i]);
    }
    
    for (int i = 0; i < num_threads; i++) 
    {
        pthread_join(threads[i], nullptr);
        sum += thread_data[i].partial_sum;
    }
    
    return sum * h;
}

double trapezoidal_openmp(double a, double b, int n) 
{
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    
    #pragma omp parallel for reduction(+:sum)
    for (int i = 1; i < n; i++) 
    {
        double x = a + i * h;
        sum += f(x);
    }
    
    return sum * h;
}

double runge_rule_sequential(double a, double b, double epsilon) 
{
    int n = 2;
    double I_prev = trapezoidal_sequential(a, b, n);
    double I_curr;
    
    do 
    {
        n *= 2;
        I_curr = trapezoidal_sequential(a, b, n);
        double error = fabs(I_curr - I_prev) / 3.0;
        
        if (error < epsilon || n > 1000000) 
        {
            break;
        }
        
        I_prev = I_curr;
    } 
    while (true);
    
    return I_curr;
}

double runge_rule_pthreads(double a, double b, double epsilon, int num_threads) 
{
    int n = 2;
    double I_prev = trapezoidal_pthreads(a, b, n, num_threads);
    double I_curr;
    
    do 
    {
        n *= 2;
        I_curr = trapezoidal_pthreads(a, b, n, num_threads);
        double error = fabs(I_curr - I_prev) / 3.0;
        
        if (error < epsilon || n > 1000000) 
        {
            break;
        }
        
        I_prev = I_curr;
    } 
    while (true);
    
    return I_curr;
}

double runge_rule_openmp(double a, double b, double epsilon) 
{
    int n = 2;
    double I_prev = trapezoidal_openmp(a, b, n);
    double I_curr;
    
    do 
    {
        n *= 2;
        I_curr = trapezoidal_openmp(a, b, n);
        double error = fabs(I_curr - I_prev) / 3.0;
        
        if (error < epsilon || n > 1000000) 
        {
            break;
        }
        
        I_prev = I_curr;
    } 
    while (true);
    
    return I_curr;
}

int main() 
{
    double a = 0.0;
    double b = 1.6;

    long double epsilon;

    int num_ants_1, num_ants_2;

    std::cout << "Введите требуемую точность (Задания 1-4) (Например: 0.0000000000001): ";
    std::cin >> epsilon;
    std::cout << "Через пробел укажите количество муравьев для последовательной и параллельной вычислений соответсвенно (Задание 5) (Например: 64 128): ";
    std::scanf("%d %d", &num_ants_1, &num_ants_2);

    int N = 0;
    double* timeMat = NULL;
    const char** names = NULL;
    build_vienna_graph(&N, &timeMat, &names);

    int start = 8;
    int target = 14;

    ACOParams params;
    params.iterations = 200;
    params.num_ants   = num_ants_1;
    params.alpha = 1.0;
    params.beta  = 3.0;
    params.rho   = 0.5;
    params.Q     = 100.0;
    params.tau0  = 1.0;
    params.tau_min = 1e-6;
    params.tau_max = 10.0;
    params.seed  = 0;

    int* bestPath = (int*)malloc(sizeof(int) * N);
    int bestLen = 0;
    double bestCost = INF_COST;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_def       = std::chrono::high_resolution_clock::now();
    long double integral_def                                                    = runge_rule_sequential(a, b, epsilon);
    std::chrono::time_point<std::chrono::high_resolution_clock> end_def         = std::chrono::high_resolution_clock::now();

    std::chrono::time_point<std::chrono::high_resolution_clock> start_pthreads  = std::chrono::high_resolution_clock::now();
    long double integral_pthreads                                               = runge_rule_pthreads(a, b, epsilon, omp_get_max_threads());
    std::chrono::time_point<std::chrono::high_resolution_clock> end_pthreads    = std::chrono::high_resolution_clock::now();

    std::chrono::time_point<std::chrono::high_resolution_clock> start_omp       = std::chrono::high_resolution_clock::now();
    long double integral_omp                                                    = runge_rule_openmp(a, b, epsilon);
    std::chrono::time_point<std::chrono::high_resolution_clock> end_omp         = std::chrono::high_resolution_clock::now();

    std::chrono::microseconds duration_def                                      = std::chrono::duration_cast<std::chrono::microseconds>(end_def         - start_def);
    std::chrono::microseconds duration_pthreads                                 = std::chrono::duration_cast<std::chrono::microseconds>(end_pthreads    - start_pthreads);
    std::chrono::microseconds duration_omp                                      = std::chrono::duration_cast<std::chrono::microseconds>(end_omp         - start_omp);

    std::printf("\n\nЗадания 1-4\n");
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------\n";
    std::cout << "| Название\t\t|\tРезультат\t|\tВремя (микросекунд)\t|\tТочность\t|\tКонтрольное значение |\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------\n";
    std::cout << "| Последовательный\t|\t" << integral_def << "\t\t|\t\t" << duration_def.count() << "\t\t|\t" << epsilon << "\t\t|\t" << "\t4.60078      |\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------\n";
    std::cout << "| Pthreads\t\t|\t" << integral_pthreads << "\t\t|\t\t" << duration_pthreads.count() << "\t\t|\t" << epsilon << "\t\t|\t" << "\t4.60078      |\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------\n";
    std::cout << "| OpenMP\t\t|\t" << integral_omp << "\t\t|\t\t" << duration_omp.count() << "\t\t|\t" << epsilon << "\t\t|\t" << "\t4.60078      |\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------\n";



    std::printf("\n\nЗадание 5\n");
    printf("Последовательная версия ACO\n");
    uint64_t t0 = now_ns();
    aco_shortest_path_sequential(N, timeMat, start, target, &params, bestPath, &bestLen, &bestCost);
    uint64_t t1 = now_ns();
    print_path(names, bestPath, bestLen, bestCost);
    printf("Время: %.3f мс\n\n", (t1 - t0) / 1e6);

    params.num_ants = num_ants_2;

    bestLen = 0;
    bestCost = INF_COST;
    printf("Параллельная версия ACO (pthreads, %d потоков)\n", omp_get_max_threads());
    t0 = now_ns();
    aco_shortest_path_parallel_pthreads(N, timeMat, start, target, &params, omp_get_max_threads(), bestPath, &bestLen, &bestCost);
    t1 = now_ns();
    print_path(names, bestPath, bestLen, bestCost);
    printf("Время: %.3f мс\n", (t1 - t0) / 1e6);

    free(bestPath);
    free((void*)names);
    free(timeMat);

    return 0;
}