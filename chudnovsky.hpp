#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

#include "utils.hpp"

#include <gmpxx.h>
#include <boost/thread/sync_queue.hpp>

class Chudnovsky {
    // constants for Chudnovsky Algorithm
    mpz_class A_, B_, C_, D_, E_, C3_24_;
    int DIGITS_, PREC_, N_, VERSION_;
    double DIGITS_PER_TERM_;

    // for concurrency
    volatile bool terminated;
    bool debug;
    int NUM_OF_CORES_, BATCH_SIZE_, BATCH_NUM_;
    boost::sync_queue<ReqPack> req_pack_q;
    boost::sync_queue<RespPack> comb_resp_pack_q;
    boost::sync_queue<RespPack> comp_resp_pack_q;
    boost::sync_queue<RespPack> comp2_resp_pack_q;
    boost::sync_queue<ReqPack> final_req_pack_q;
    boost::sync_queue<RespPack> final_resp_pack_q;

    std::vector<std::thread> pqt_workers;
    std::thread pi_worker;

    void PIWorker();
    // Version 0 Entry.
    PQT ComputePQT(int n1, int n2);
    // Version 1 Entry.
    PQT PQTMasterV1();
    // Version 2 Entry.
    PQT PQTMasterV2();
    // Version 3 Entry.
    PQT PQTMasterV3();

    // Version 1 Impl.
    PQT ComputePQTMasterV1();
    RespPack CombinePQTMasterV1(RespPack& rp1, RespPack& rp2);
    void PQTWorkerV1();

    // Version 2 Impl.
    PQT ComputePQTMasterV2();
    void CombinePQTMasterV2(std::vector<RespPack>& resp_packs, size_t resp_packs_size);
    void CombinePQTSenderV2(RespPack& rp1, RespPack& rp2);
    PQT CombinePQTMergerV2(std::vector<RespPack>& resp_packs, int sliding_window_begin);
    bool CombinePQTCheckResultV2(std::vector<RespPack>& resp_packs, int sliding_window_begin, int sliding_window_end);

    // Version 3 Impl.
    PQT ComputePQTMasterV3();
    void CombinePQTMasterV3(std::vector<RespPack>& resp_packs, size_t resp_packs_size);
    void Combine2PQTSenderV3(int id, std::vector<RespPack>& resp_packs, int index);

public:
    Chudnovsky() = delete;
    Chudnovsky(int version, int digits, int worker_num);
    ~Chudnovsky();

    void Start(bool nout);
    void StartConcurrent(bool nout);
    void Stop();
};
