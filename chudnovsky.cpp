#include "chudnovsky.hpp"

Chudnovsky::Chudnovsky(int version, int digits, int worker_num): terminated(false), debug(false) {
    VERSION_ = version;
    // constants for Chudnovsky Algorithm
    DIGITS_ = std::max(digits, 0);
    A_ = 13591409;
    B_ = 545140134;
    C_ = 640320;
    D_ = 426880;
    E_ = 10005;
    // = log(53360^3) / log(10)
    DIGITS_PER_TERM_ = 14.1816474627254776555;
    C3_24_ = C_ * C_ * C_ / 24;
    N_ = std::max(DIGITS_PER_TERM_, static_cast<double>(DIGITS_)) / DIGITS_PER_TERM_;
    PREC_ = std::max(0, DIGITS_) * log2(10);

    // for concurrency
    int cpu_no = 0;

    NUM_OF_CORES_ = worker_num <= 0 ? std::thread::hardware_concurrency() : worker_num;
    pqt_workers = std::vector<std::thread>(std::max(NUM_OF_CORES_, 1));
    for (auto& pqt_worker: pqt_workers) {
        pqt_worker = std::thread(&Chudnovsky::PQTWorkerV1, this);
        SetCpuAffinity((cpu_no++)%NUM_OF_CORES_, pqt_worker);
    }
    SetCpuAffinity((cpu_no++)%NUM_OF_CORES_);

    pi_worker = std::thread(&Chudnovsky::PIWorker, this);
    SetCpuAffinity((cpu_no++)%NUM_OF_CORES_, pi_worker);
}

Chudnovsky::~Chudnovsky() {
    Stop();
}

void Chudnovsky::Stop() {
    if (terminated) return;
    
    terminated = true;

    for (int i = 0; i < pqt_workers.size(); i++) {
        req_pack_q.push(ReqPack());
    }

    for (auto& pqt_worker: pqt_workers) {
        pqt_worker.join();
    }

    final_req_pack_q.push(ReqPack());
    pi_worker.join();
}

/*
 * Version 0:
 * Chudnovsky Algorithm in single thread mode
 * This will be used as base method in the further versions
 */
PQT Chudnovsky::ComputePQT(int n1, int n2) {
    int mid;
    PQT res;

    if (n1 + 1 == n2) {
        res.P  = (2 * n2 - 1);
        res.P *= (6 * n2 - 1);
        res.P *= (6 * n2 - 5);
        res.Q  = C3_24_ * n2 * n2 * n2;
        res.T  = (A_ + B_ * n2) * res.P;
        if ((n2 & 1) == 1) res.T = - res.T;
    } else {
        mid = (n1 + n2) / 2;
        PQT res1 = ComputePQT(n1, mid);
        PQT res2 = ComputePQT(mid, n2);
        res.P = res1.P * res2.P;
        res.Q = res1.Q * res2.Q;
        res.T = res1.T * res2.Q + res1.P * res2.T;
    }

    return res;
}

/*
 * Version 1:
 * Part 1. PQTMasterV1() distribute ReqPack into PQTWorkerV1(), and wait for merge the result.
 *         Continuously receive RespPack from PQTWorkerV1(), then we can start to run CombinePQTMasterV1() "one by one".
 * Part 2. CombinePQTMasterV1() distribute the ReqPack to PQTWorkerV1(), which do the MP multiplication in multithread.
 *         After combined the RespPack, return it back to ComputePQTMasterV1() for further distribution.
 * Part 3. Only 1 RespPack left in the end, and that is the result.
 */
PQT Chudnovsky::PQTMasterV1() {
    // pack the request
    int begin = 0, end = BATCH_SIZE_;
    for (int i = 0; i < BATCH_NUM_; i++) {
        req_pack_q.push(ReqPack(i, begin, end));
        begin = end;
        end += BATCH_SIZE_;
    }

    return ComputePQTMasterV1();
}

/*
 */
PQT Chudnovsky::ComputePQTMasterV1() {
    // prepare for response
    RespPack resp_pack;
    std::vector<RespPack> resp_packs = std::vector<RespPack>(BATCH_NUM_);
    int sliding_window_begin = 0, sliding_window_end = 1;
    size_t resp_packs_size = resp_packs.size();
    while (!terminated) {
        // block at queue
        comp_resp_pack_q.pull(resp_pack);
        resp_packs[resp_pack.GetID()] = resp_pack;

        // check if we can do a CombinePQT()
        while (sliding_window_end < resp_packs_size && resp_packs[sliding_window_begin].IsValid() && resp_packs[sliding_window_end].IsValid()) {
            resp_packs[sliding_window_begin/2] = CombinePQTMasterV1(resp_packs[sliding_window_begin], resp_packs[sliding_window_end]);

            if (sliding_window_end != resp_packs_size-1) {
                sliding_window_begin += 2;
                sliding_window_end += 2;
                continue;
            }

            sliding_window_begin = 0;
            sliding_window_end = 1;
            resp_packs_size >>= 1;
        }
        
        // check if all workers are done, if so terminated = true, send terminate ReqPack to workers, then break myself
        if (resp_packs_size == 1) break;
    }

    return resp_packs[resp_packs_size-1].GetResult();
}

/*
 */
RespPack Chudnovsky::CombinePQTMasterV1(RespPack& resp_pack1, RespPack& resp_pack2) {
    RespPack resp_pack;
    PQT res;
    PQT res1 = resp_pack1.GetResult();
    PQT res2 = resp_pack2.GetResult();

    req_pack_q.push(ReqPack(0, res1.P, res2.P));
    req_pack_q.push(ReqPack(1, res1.Q, res2.Q));
    req_pack_q.push(ReqPack(2, res1.T, res2.Q));
    req_pack_q.push(ReqPack(3, res1.P, res2.T));

    // currently do the combining sequentially, and do it one by one
    std::vector<RespPack> resp_packs = std::vector<RespPack>(4);
    for (int i = 0; i < 4; i++) {
        comb_resp_pack_q.pull(resp_pack);
        resp_packs[resp_pack.GetID()] = resp_pack;
    }

    res.P = resp_packs[0].Geta();
    res.Q = resp_packs[1].Geta();
    res.T = resp_packs[2].Geta() + resp_packs[3].Geta();

    return RespPack(resp_pack1.GetID()/2, resp_pack1.GetN1(), resp_pack2.GetN2(), res);
}

void Chudnovsky::PQTWorkerV1() {
    ReqPack req_pack;
    while (!terminated) {
        // block at queue
        req_pack_q.pull(req_pack);

        // check if terminiated
        if (!req_pack.IsValid()) break;

        if (req_pack.GetType() == TYPE_COMPUTE) {
            // do ComputePQT()
            PQT res = ComputePQT(req_pack.GetN1(), req_pack.GetN2());

            // generate a RespPack
            RespPack resp_pack(req_pack, res);

            // push a RespPack
            comp_resp_pack_q.push(resp_pack);
        } else if (req_pack.GetType() == TYPE_COMBINE) {
            // do mpz multiplicate
            mpz_class res = req_pack.Geta() * req_pack.Getb();

            // generate a RespPack
            RespPack resp_pack(req_pack, res);

            // push a RespPack
            comb_resp_pack_q.push(resp_pack);
        } else if (req_pack.GetType() == TYPE_COMBINE2) {
            PQT res;

            // do combination
            res.P = req_pack.Geta();
            res.Q = req_pack.Getb();
            res.T = req_pack.Getc() + req_pack.Getd();

            // generate a RespPack
            RespPack resp_pack(req_pack, res);

            // push a RespPack
            comp_resp_pack_q.push(resp_pack);
        }
    }
}

/* 
 * Version 2:
 * Part 1. PQTMasterV2() distribute ReqPack into PQTWorkerV1(), and wait for merge the result.
 *         Continuously receive RespPack from PQTWorkerV1(), different from version 1, when sliding_window ready, call CombinePQTSenderV2() to preprocess before CombinePQTMasterV2().
 * Part 1.5. CombinePQTSenderV2() will pack each multiplication operation into ReqPack, and send it to PQTWorkerV1() to calculate it without waiting on the result "one by one". This method "eliminate most of the blocking period in the previous version".
 * Part 2. After the preprocess, CombinePQTMasterV2() and CombinePQTMergerV2() will only do the add operation, then store the result back to ComputePQTMasterV2().
 * Part 3. Only 1 RespPack left in the end, and that is the result.
 */
PQT Chudnovsky::PQTMasterV2() {
    // pack the request
    int begin = 0, end = BATCH_SIZE_;
    for (int i = 0; i < BATCH_NUM_; i++) {
        req_pack_q.push(ReqPack(i, begin, end));
        begin = end;
        end += BATCH_SIZE_;
    }

    return ComputePQTMasterV2();
}

/*
 */
PQT Chudnovsky::ComputePQTMasterV2() {
    // prepare for response
    RespPack resp_pack;
    std::vector<RespPack> resp_packs = std::vector<RespPack>(BATCH_NUM_);
    int sliding_window_begin = 0, sliding_window_end = 1;
    size_t resp_packs_size = resp_packs.size();
    while (!terminated) {
        // block at queue
        comp_resp_pack_q.pull(resp_pack);
        resp_packs[resp_pack.GetID()] = resp_pack;

        // check if we can prepare to combine the result
        while (sliding_window_end < resp_packs_size && resp_packs[sliding_window_begin].IsValid() && resp_packs[sliding_window_end].IsValid()) {
            // the function to send ReqPack
            CombinePQTSenderV2(resp_packs[sliding_window_begin], resp_packs[sliding_window_end]);

            // if we finished the first compute part, start the second - combine part
            if (sliding_window_end != resp_packs_size-1) {
                // slide the window, which contains the current focused RespPack that is going to combined
                sliding_window_begin += 2;
                sliding_window_end += 2;
                continue;
            }

            resp_packs_size >>= 1;
            CombinePQTMasterV2(resp_packs, resp_packs_size);

            sliding_window_begin = 0;
            sliding_window_end = 1;
        }
        
        // check if all task are done, if so break myself
        if (resp_packs_size == 1) break;
    }

    return resp_packs[resp_packs_size-1].GetResult();
}

/*
 */
void Chudnovsky::CombinePQTMasterV2(std::vector<RespPack>& parent_resp_packs, size_t resp_packs_size) {
    RespPack resp_pack;
    std::vector<RespPack> resp_packs = std::vector<RespPack>(4*resp_packs_size);
    int sliding_window_begin = 0, sliding_window_end = 3;
    resp_packs_size = resp_packs.size();
    while (!terminated) {
        comb_resp_pack_q.pull(resp_pack);
        resp_packs[resp_pack.GetID()] = resp_pack;

        while (sliding_window_end < resp_packs_size && CombinePQTCheckResultV2(resp_packs, sliding_window_begin, sliding_window_end)) {
            int id = sliding_window_begin >> 2;
            parent_resp_packs[id] = RespPack(id, parent_resp_packs[id].GetN1(), parent_resp_packs[id+1].GetN2(), CombinePQTMergerV2(resp_packs, sliding_window_begin));

            if (sliding_window_end != resp_packs_size-1) {
                sliding_window_begin += 4;
                sliding_window_end += 4;
                continue;
            }

            return;
        }
    }
}

/* 
 */
PQT Chudnovsky::CombinePQTMergerV2(std::vector<RespPack>& resp_packs, int sliding_window_begin) {
    PQT res;

    res.P = resp_packs[sliding_window_begin].Geta();
    res.Q = resp_packs[sliding_window_begin+1].Geta();
    res.T = resp_packs[sliding_window_begin+2].Geta() + resp_packs[sliding_window_begin+3].Geta();

    return res;
}

/*
 */
bool Chudnovsky::CombinePQTCheckResultV2(std::vector<RespPack>& resp_packs, int begin, int end) {
    for (int i = begin; i <= end; i++) {
        if (!resp_packs[i].IsValid()) return false;
    }

    return true;
}

/* 
 */
void Chudnovsky::CombinePQTSenderV2(RespPack& resp_pack1, RespPack& resp_pack2) {
    PQT res1 = resp_pack1.GetResult();
    PQT res2 = resp_pack2.GetResult();
    int res_id_base = resp_pack1.GetID()*2;

    req_pack_q.push(ReqPack(res_id_base+0, res1.P, res2.P));
    req_pack_q.push(ReqPack(res_id_base+1, res1.Q, res2.Q));
    req_pack_q.push(ReqPack(res_id_base+2, res1.T, res2.Q));
    req_pack_q.push(ReqPack(res_id_base+3, res1.P, res2.T));

    resp_pack1.Invalidate();
    resp_pack2.Invalidate();
}
/*
 */

PQT Chudnovsky::PQTMasterV3() {
    // pack the request
    int begin = 0, end = BATCH_SIZE_;
    for (int i = 0; i < BATCH_NUM_; i++) {
        req_pack_q.push(ReqPack(i, begin, end));
        begin = end;
        end += BATCH_SIZE_;
    }

    return ComputePQTMasterV3();
}

/*
 */
PQT Chudnovsky::ComputePQTMasterV3() {
    // prepare for response
    RespPack resp_pack;
    std::vector<RespPack> resp_packs = std::vector<RespPack>(BATCH_NUM_);
    int sliding_window_begin = 0, sliding_window_end = 1;
    size_t resp_packs_size = resp_packs.size();
    while (!terminated) {
        // block at queue
        comp_resp_pack_q.pull(resp_pack);
        resp_packs[resp_pack.GetID()] = resp_pack;

        // check if we can prepare to combine the result
        while (sliding_window_end < resp_packs_size && resp_packs[sliding_window_begin].IsValid() && resp_packs[sliding_window_end].IsValid()) {
            // the function to send ReqPack
            CombinePQTSenderV2(resp_packs[sliding_window_begin], resp_packs[sliding_window_end]);

            // if we finished the first compute part, start the second - combine part
            if (sliding_window_end != resp_packs_size-1) {
                // slide the window, which contains the current focused RespPack that is going to combined
                sliding_window_begin += 2;
                sliding_window_end += 2;
                continue;
            }

            resp_packs_size >>= 1;
            CombinePQTMasterV3(resp_packs, resp_packs_size);

            sliding_window_begin = 0;
            sliding_window_end = 1;

            break;
        }
        
        // check if all task are done, if so break myself
        if (resp_packs_size == 1 && resp_packs[0].IsValid()) break;
    }

    return resp_packs[resp_packs_size-1].GetResult();
}

/*
 */
void Chudnovsky::CombinePQTMasterV3(std::vector<RespPack>& parent_resp_packs, size_t resp_packs_size) {
    RespPack resp_pack;
    std::vector<RespPack> resp_packs = std::vector<RespPack>(4*resp_packs_size);
    int sliding_window_begin = 0, sliding_window_end = 3;
    resp_packs_size = resp_packs.size();
    while (!terminated) {
        comb_resp_pack_q.pull(resp_pack);
        resp_packs[resp_pack.GetID()] = resp_pack;

        while (sliding_window_end < resp_packs_size && CombinePQTCheckResultV2(resp_packs, sliding_window_begin, sliding_window_end)) {
            int id = sliding_window_begin >> 2;
            Combine2PQTSenderV3(id, resp_packs, sliding_window_begin);

            if (sliding_window_end != resp_packs_size-1) {
                sliding_window_begin += 4;
                sliding_window_end += 4;
                continue;
            }

            return;
        }
    }
}

/* 
 */
void Chudnovsky::Combine2PQTSenderV3(int id, std::vector<RespPack>& resp_packs, int index) {
    req_pack_q.push(ReqPack(id, resp_packs[index].Geta(), resp_packs[index+1].Geta(), resp_packs[index+2].Geta(), resp_packs[index+3].Geta()));

    resp_packs[index].Invalidate();
    resp_packs[index+1].Invalidate();
    resp_packs[index+2].Invalidate();
    resp_packs[index+3].Invalidate();
}

/*
 * Version 3:
 */
void Chudnovsky::PIWorker() {
    ReqPack req_pack;

    while (!terminated) {
        // wait for signal
        final_req_pack_q.pull(req_pack);
        mpf_class res(sqrt((mpf_class)E_), PREC_);
        final_resp_pack_q.push(RespPack(req_pack, res));
    }
}

/*
 * Compute PI: Single Thread
 */
void Chudnovsky::Start(bool nout) {
    // BATCH_NUM must be the power of 2, get the leftmost bit
    BATCH_NUM_ = static_cast<int>(GetBatchNum(NUM_OF_CORES_)) * 8;
    BATCH_SIZE_ = (N_ / BATCH_NUM_) + 1;

    std::cerr << " [*] PI with " << DIGITS_ << " digits" << std::endl;

    // Time (start)
    ClockStart();

    // Compute Pi
    PQT PQT = ComputePQT(0, N_);
    mpf_class pi(0, PREC_);
    pi = D_ * sqrt((mpf_class)E_) * PQT.Q;
    pi /= (A_ * PQT.Q + PQT.T);

    // Time (end of computation)
    ClockEnd(0);
    ClockStart();

    // Output // +1 for dot
    if (!nout) {
        std::ofstream ofs ("pi_normal.txt");
        ofs.precision(DIGITS_ + 1);
        ofs << pi << std::endl;
        ofs.close();
    }

    // Time (end of writing)
    ClockEnd(0);
}

/*
 * Compute PI: Multithread
 */
void Chudnovsky::StartConcurrent(bool nout) {
    // BATCH_NUM must be the power of 2, get the leftmost bit
    BATCH_NUM_ = static_cast<int>(GetBatchNum(NUM_OF_CORES_)) * 8;
    BATCH_SIZE_ = (N_ / BATCH_NUM_) + 1;

    std::cerr << " [*] PI with " << DIGITS_ << " digits" << std::endl;

    // Time (start)
    ClockStart();

    // Compute Pi
    RespPack resp_pack;
    PQT pqt;

    // Choose version
    if (VERSION_ == 1) pqt = PQTMasterV1();
    else if (VERSION_ == 2) pqt = PQTMasterV2();
    else if (VERSION_ == 3) pqt = PQTMasterV3();
    else {
        std::cerr << " [*] No such version = " << VERSION_ << std::endl;
        // Time (end because of error)
        ClockEnd(0);
        return;
    }

    // multithread this part
    final_req_pack_q.push(ReqPack());
    mpf_class F(A_ * pqt.Q + pqt.T, PREC_);
    mpf_class pi((D_ * pqt.Q) / F, PREC_);
    final_resp_pack_q.pull(resp_pack);

    pi *= resp_pack.Getfa();

    // Time (end of computation)
    ClockEnd(0);
    ClockStart();

    // Output // +1 for dot
    if (!nout) {
        std::ofstream ofs ("pi_concurrent.txt");
        ofs.precision(DIGITS_ + 1);
        ofs << pi << std::endl;
        ofs.close();
    }

    // Time (end of writing)
    ClockEnd(0);
}
