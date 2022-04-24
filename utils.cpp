#include <iostream>
#include <thread>
#include <chrono>

#include "utils.hpp"

using HRC = std::chrono::high_resolution_clock;
using HRC_PT = std::chrono::time_point<HRC>;
using MS = std::chrono::milliseconds;

ReqPack::ReqPack(): id_(-1), n1_(-1), n2_(-1), type_(TYPE_UNKNOWN) {};
ReqPack::ReqPack(int id, int n1, int n2): id_(id), n1_(n1), n2_(n2), type_(TYPE_COMPUTE) {};
ReqPack::ReqPack(int id, mpz_class& a, mpz_class& b): id_(id), a_(a), b_(b), type_(TYPE_COMBINE) {};
ReqPack::ReqPack(int id, mpf_class& fa): id_(id), fa_(fa), type_(TYPE_COMBINE) {};
ReqPack::ReqPack(int id, mpf_class& fa, mpf_class& fb): id_(id), fa_(fa), fb_(fb), type_(TYPE_COMBINE) {};
ReqPack::ReqPack(int id, mpz_class& a, mpz_class& b, mpz_class& c, mpz_class& d): id_(id), a_(a), b_(b), c_(c), d_(d), type_(TYPE_COMBINE2) {};
ReqPack::ReqPack(int id, mpz_class&& a, mpz_class&& b, mpz_class&& c, mpz_class&& d): id_(id), a_(a), b_(b), c_(c), d_(d), type_(TYPE_COMBINE2) {};
int ReqPack::GetID() {return id_;};
int ReqPack::GetN1() {return n1_;};
int ReqPack::GetN2() {return n2_;};
PackType ReqPack::GetType() {return type_;};
mpz_class ReqPack::Geta() {return a_;};
mpz_class ReqPack::Getb() {return b_;};
mpz_class ReqPack::Getc() {return c_;};
mpz_class ReqPack::Getd() {return d_;};
mpf_class ReqPack::Getfa() {return fa_;};
mpf_class ReqPack::Getfb() {return fb_;};
void ReqPack::Invalidate() {id_ = -1;};
bool ReqPack::IsValid() {return id_ != -1;};

RespPack::RespPack(): id_(-1), n1_(-1), n2_(-1), result_({}), type_(TYPE_UNKNOWN) {};
RespPack::RespPack(int id, int n1, int n2, PQT& result): id_(id), n1_(n1), n2_(n2), result_(result), type_(TYPE_COMPUTE) {};
RespPack::RespPack(int id, int n1, int n2, PQT&& result): id_(id), n1_(n1), n2_(n2), result_(result), type_(TYPE_COMPUTE) {};
RespPack::RespPack(ReqPack& req_pack, PQT& result): id_(req_pack.GetID()), n1_(req_pack.GetN1()), n2_(req_pack.GetN2()), result_(result), type_(req_pack.GetType()) {};
RespPack::RespPack(ReqPack& req_pack, PQT&& result): id_(req_pack.GetID()), n1_(req_pack.GetN1()), n2_(req_pack.GetN2()), result_(result), type_(req_pack.GetType()) {};
RespPack::RespPack(int id, mpz_class& a): id_(id), a_(a), type_(TYPE_COMBINE) {};
RespPack::RespPack(ReqPack& req_pack, mpz_class& a): id_(req_pack.GetID()), a_(a), type_(req_pack.GetType()) {};
RespPack::RespPack(ReqPack& req_pack, mpf_class& fa): id_(req_pack.GetID()), fa_(fa), type_(req_pack.GetType()) {};
int RespPack::GetID() {return id_;};
int RespPack::GetN1() {return n1_;};
int RespPack::GetN2() {return n2_;};
PQT RespPack::GetResult() {return result_;};
PackType RespPack::GetType() {return type_;};
mpz_class RespPack::Geta() {return a_;};
mpf_class RespPack::Getfa() {return fa_;};
void RespPack::Invalidate() {id_ = -1;};
bool RespPack::IsValid() {return id_ != -1;};

static HRC_PT t_start;
static HRC_PT t_end;

void ClockStart() {
    t_start = HRC::now();
}

void ClockEnd(int ms) {
    t_end = HRC::now();
    auto latency = std::chrono::duration_cast<MS>(t_end-t_start);
    if (latency.count() > MS{ms}.count()) {
        std::cerr << " [O] Time Elapsed(ms): " << latency.count() << std::endl;
    }
}


void SetCpuAffinity(int cpu_no, std::thread& thread_obj) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_no, &cpu_set);

    pthread_setaffinity_np(thread_obj.native_handle(), sizeof(cpu_set), &cpu_set);
}

void SetCpuAffinity(int cpu_no) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_no, &cpu_set);

    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
}

uint32_t GetBatchNum(int num_of_cores) {
    uint32_t res = num_of_cores;

    res |= (res >> 1);
    res |= (res >> 2);
    res |= (res >> 4);
    res |= (res >> 8);
    res |= (res >> 16);

    return res - (res >> 1);
}
