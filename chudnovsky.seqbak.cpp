/***************************************************************
 * Computing pi by Binary Splitting Algorithm with GMP libarary.
 **************************************************************/
#include <cmath>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <gmpxx.h>
#include <boost/thread/sync_queue.hpp>

using namespace std;

struct PQT {
    mpz_class P, Q, T;
};

class ReqPack {
    int _id;
    int _n1;
    int _n2;
    int _type;
    mpz_class _a, _b;
public:
    ReqPack(): _id(-1), _n1(-1), _n2(-1), _type(-1) {};
    ReqPack(int id, int n1, int n2): _id(id), _n1(n1), _n2(n2), _type(0) {};
    ReqPack(int id, int n1, int n2, int type): _id(id), _n1(n1), _n2(n2), _type(type) {};
    ReqPack(int id, mpz_class& a, mpz_class& b): _id(id), _a(a), _b(b) {};
    int getID() {return _id;}
    int getN1() {return _n1;}
    int getN2() {return _n2;}
    int getType() {return _type;};
    mpz_class geta() {return _a;};
    mpz_class getb() {return _b;};
};

class RespPack {
    int _id;
    int _n1;
    int _n2;
    PQT _result;
    int _type;
    mpz_class _a;
public:
    RespPack(): _id(-1), _n1(-1), _n2(-1), _result({}), _type(-1) {};
    RespPack(int id, int n1, int n2, PQT result): _id(id), _n1(n1), _n2(n2), _result(result), _type(0) {};
    RespPack(int id, int n1, int n2, PQT result, int type): _id(id), _n1(n1), _n2(n2), _result(result), _type(type) {};
    RespPack(ReqPack& rp, PQT result): _id(rp.getID()), _n1(rp.getN1()), _n2(rp.getN2()), _result(result), _type(rp.getType()) {};
    RespPack(int id, mpz_class& a): _id(id), _a(a) {};
    RespPack(ReqPack& rp, mpz_class a): _id(rp.getID()), _a(a) {};
    int getID() {return _id;}
    int getN1() {return _n1;}
    int getN2() {return _n2;}
    PQT getResult() {return _result;};
    int getType() {return _type;};
    mpz_class geta() {return _a;};
};

void set_cpu_affinity(int cpu_no, thread& thread_obj) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_no, &cpu_set);

    pthread_setaffinity_np(thread_obj.native_handle(), sizeof(cpu_set), &cpu_set);
}

void set_cpu_affinity(int cpu_no) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_no, &cpu_set);

    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
}

class Chudnovsky {
    // Declaration
    mpz_class A, B, C, D, E, C3_24;     // GMP Integer
    int DIGITS, PREC, N;                // Integer
    double DIGITS_PER_TERM;             // Long
    clock_t t0, t1, t2;                 // Time
    PQT compPQT(int n1, int n2);        // Computer PQT (by BSA)

    // for concurrency
    volatile bool ready, terminated;
    mutex mtx;
    condition_variable cond_var;
    int NUM_OF_CORES, BATCH_SIZE, BATCH_NUM;
    boost::sync_queue<ReqPack> comp_reqp_q;
    boost::sync_queue<RespPack> comp_respp_q;
    boost::sync_queue<ReqPack> comb_reqp_q;
    boost::sync_queue<RespPack> comb_respp_q;
    vector<thread> combPQTworkers, compPQTworkers;
    PQT compPQTMaster();
    void compPQTWorker();
    void combPQTWorker();
    RespPack combPQT(RespPack& rp1, RespPack& rp2);

public:
    Chudnovsky() = delete;
    Chudnovsky(int digits, int batch_num, int worker_num);             // Constructor
    ~Chudnovsky();
    void start();
    void startConcurrently();
};

/*
 * Constructor
 */
Chudnovsky::Chudnovsky(int digits, int batch_num, int worker_num): ready(false), terminated(false) {
    // for concurrency
    int cpu_no = 0;
    
    NUM_OF_CORES = worker_num <= 0 ? thread::hardware_concurrency() : worker_num;
    compPQTworkers = vector<thread>(NUM_OF_CORES);
    combPQTworkers = vector<thread>(NUM_OF_CORES);
    for (auto& compPQTworker: compPQTworkers) {
        compPQTworker = thread(&Chudnovsky::compPQTWorker, this);
        set_cpu_affinity(cpu_no++, compPQTworker);
    }

    cpu_no = 0;
    for (auto& combPQTworker: combPQTworkers) {
        combPQTworker = thread(&Chudnovsky::combPQTWorker, this);
        set_cpu_affinity(cpu_no++, combPQTworker);
    }

    // Constants
    DIGITS = digits;
    A      = 13591409;
    B      = 545140134;
    C      = 640320;
    D      = 426880;
    E      = 10005;
    DIGITS_PER_TERM = 14.1816474627254776555;  // = log(53360^3) / log(10)
    C3_24  = C * C * C / 24;
    N      = max(DIGITS_PER_TERM, static_cast<double>(DIGITS)) / DIGITS_PER_TERM;
    PREC   = max(0, DIGITS) * log2(10);

    // for concurrency
    BATCH_NUM = batch_num;
    BATCH_SIZE = N/BATCH_NUM + 1;
}

Chudnovsky::~Chudnovsky() {
    ready = terminated = true;
    for (int i = 0; i < compPQTworkers.size(); i++) {
        comp_reqp_q.push(ReqPack());
    }

    for (int i = 0; i < combPQTworkers.size(); i++) {
        comb_reqp_q.push(ReqPack());
    }

    for (auto& compPQTworker: compPQTworkers) {
        compPQTworker.join();
    }

    for (auto& combPQTworker: combPQTworkers) {
        combPQTworker.join();
    }
}

PQT Chudnovsky::compPQTMaster() {
    // pack the request
    int begin = 0, end = BATCH_SIZE;
    for (int i = 0; i < BATCH_NUM; i++) {
        comp_reqp_q.push(ReqPack(i, begin, end));
        begin = end;
        end += BATCH_SIZE;
    }

    // ready!
    ready = true;
    cerr << "Master Ready" << endl;

    // prepare for response
    // NOTICE: BATCH_NUM must be power of 2
    vector<RespPack> respps = vector<RespPack>(BATCH_NUM);
    int sliding_window_begin = 0, sliding_window_end = 1;
    size_t respps_size = respps.size();
    while (!terminated) {
        // block at queue
        RespPack respp;
        comp_respp_q.pull(respp);
        respps[respp.getID()] = respp;

        // check if we can do a combPQT()
        while (sliding_window_end < respps_size && respps[sliding_window_begin].getID() != -1 && respps[sliding_window_end].getID() != -1) {
            respps[sliding_window_end] = combPQT(respps[sliding_window_begin], respps[sliding_window_end]);

            cerr << sliding_window_begin << ":" << sliding_window_end << ":" << respps_size << endl;
            if (sliding_window_end == respps_size-1) break;

            sliding_window_begin += 1;
            sliding_window_end += 1;
        }
        
        // check if all workers are done, if so terminated = true, send terminate ReqPack to workers, then break myself
        if (sliding_window_end == respps_size-1) break;
    }

    cerr << "Master Terminated" << endl;

    return respps.back().getResult();
}

void Chudnovsky::compPQTWorker() {
    while (!ready) sleep(1);
    //cerr << "Worker Ready" << endl;

    // split into batches according to BATCH_SIZE and NUM_OF_CORES
    // use boost::queue to get batch from master

    while (!terminated) {
        // block at queue
        ReqPack reqp;
        comp_reqp_q.pull(reqp);
        //cerr << "Worker Got a Pack: " << reqp.getID() << endl;

        // check if terminiated
        if (reqp.getID() == -1) break;

        // do compPQT()
        PQT res = compPQT(reqp.getN1(), reqp.getN2());
        //cerr << "Worker Done a Pack: " << reqp.getID() << endl;

        // generate a RespPack
        RespPack respp(reqp, res);

        // push a RespPack
        comp_respp_q.push(respp);
        //cerr << "Worker Send a Pack: " << reqp.getID() << endl;
    }

    cerr << "Worker Terminated" << endl;
}

void Chudnovsky::combPQTWorker() {
    while (!ready) sleep(1);

    while (!terminated) {
        ReqPack reqp;
        comb_reqp_q.pull(reqp);

        if (reqp.getID() == -1) break;

        mpz_class res = reqp.geta() * reqp.getb();

        RespPack respp(reqp, res);

        comb_respp_q.push(respp);
    }

    cerr << "Worker Terminated" << endl;
}

/*
 * combine into rp2
 */
RespPack Chudnovsky::combPQT(RespPack& rp1, RespPack& rp2) {
    PQT res;
    PQT res1 = rp1.getResult();
    PQT res2 = rp2.getResult();

    comb_reqp_q.push(ReqPack(0, res1.P, res2.P));
    comb_reqp_q.push(ReqPack(1, res1.Q, res2.Q));
    comb_reqp_q.push(ReqPack(2, res1.T, res2.Q));
    comb_reqp_q.push(ReqPack(3, res1.P, res2.T));

    vector<RespPack> respps = vector<RespPack>(4);
    for (int i = 0; i < 4; i++) {
        RespPack respp;
        comb_respp_q.pull(respp);
        respps[respp.getID()] = respp;
    }

    res.P = respps[0].geta();
    res.Q = respps[1].geta();
    res.T = respps[2].geta() + respps[3].geta();

    return RespPack(rp2.getID()/2, rp1.getN1(), rp2.getN2(), res);
}

/*
 * Compute PQT (by Binary Splitting Algorithm)
 */
PQT Chudnovsky::compPQT(int n1, int n2) {
    int m;
    PQT res;

    if (n1 + 1 == n2) {
        res.P  = (2 * n2 - 1);
        res.P *= (6 * n2 - 1);
        res.P *= (6 * n2 - 5);
        res.Q  = C3_24 * n2 * n2 * n2;
        res.T  = (A + B * n2) * res.P;
        if ((n2 & 1) == 1) res.T = - res.T;
    } else {
        m = (n1 + n2) / 2;
        PQT res1 = compPQT(n1, m);
        PQT res2 = compPQT(m, n2);
        res.P = res1.P * res2.P;
        res.Q = res1.Q * res2.Q;
        res.T = res1.T * res2.Q + res1.P * res2.T;
    }

    return res;
}

/*
 * Compute PI
 */
void Chudnovsky::start() {
    cout << "**** PI Computation ( " << DIGITS << " digits )" << endl;

    // Time (start)
    t0 = clock();

    // Compute Pi
    PQT PQT = compPQT(0, N);
    mpf_class pi(0, PREC);
    pi  = D * sqrt((mpf_class)E) * PQT.Q;
    pi /= (A * PQT.Q + PQT.T);

    // Time (end of computation)
    t1 = clock();
    cout << "TIME (COMPUTE): "
         << (double)(t1 - t0) / CLOCKS_PER_SEC
         << " seconds." << endl;

    // Output // +1 for dot
    ofstream ofs ("pi_normal.txt");
    ofs.precision(DIGITS + 1);
    ofs << pi << endl;

    // Time (end of writing)
    t2 = clock();
    cout << "TIME (WRITE)  : "
         << (double)(t2 - t1) / CLOCKS_PER_SEC
         << " seconds." << endl;
}

void Chudnovsky::startConcurrently() {
    cout << "**** PI Computation ( " << DIGITS << " digits )" << endl;

    // Time (start)
    t0 = clock();

    // Compute Pi
    PQT PQT = compPQTMaster();
    mpf_class pi(0, PREC);
    pi  = D * sqrt((mpf_class)E) * PQT.Q;
    pi /= (A * PQT.Q + PQT.T);

    // Time (end of computation)
    t1 = clock();
    cout << "TIME (COMPUTE): "
         << (double)(t1 - t0) / CLOCKS_PER_SEC
         << " seconds." << endl;

    // Output // +1 for dot
    ofstream ofs ("pi_concurrent.txt");
    ofs.precision(DIGITS + 1);
    ofs << pi << endl;

    // Time (end of writing)
    t2 = clock();
    cout << "TIME (WRITE)  : "
         << (double)(t2 - t1) / CLOCKS_PER_SEC
         << " seconds." << endl;
}

int main() {
    try {
        // Instantiation
        Chudnovsky calc(3000, 4, 4);

        // single thread
        calc.start();

        // for concurrency
        calc.startConcurrently();
    }
    catch (...) {
        cout << "ERROR!" << endl;
        return -1;
    }

    return 0;
}
